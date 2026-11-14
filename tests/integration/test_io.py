import subprocess
import unittest

from observation import ObservedCompilerTestCase


class InputOutputTests(ObservedCompilerTestCase):
    def execute(self, executable, data=b""):
        return subprocess.run([str(executable)], cwd=self.directory, input=data,
                              capture_output=True, timeout=15)

    def test_numeric_text_rejects_invalid_spellings_and_ranges(self):
        cases = {
            "toInt": [b"", b"+", b"-", b"1.0", b"1e2", b"0x10", b"12junk", b"1 2",
                      b"2147483648", b"-2147483649", b"999999999999999999999999", b"\xc2\xa012"],
            "toReal": [b"", b".", b"+.", b"1e", b"1e+", b"0x1p2", b"nan", b"inf",
                       b"Infinity", b"1,5", b"1_000", b"1 2", b"1e999", b"1e-999"],
            "toBool": [b"", b"True", b"FALSE", b"1", b"true false", b"false!"],
        }
        for conversion, values in cases.items():
            self.source.write_text(f"launch() {{ show({conversion}(ask(text))); }}")
            executable = self.compile()
            for value in values:
                with self.subTest(conversion=conversion, value=value):
                    result = self.execute(executable, value + b"\n")
                    self.assertEqual(result.returncode, 1, result.stderr)
                    self.assertEqual(result.stdout, b"")
                    self.assertIn(b"input.ruv:1:17: error:", result.stderr)

    def test_typed_input_rejects_eof_and_invalid_values(self):
        cases = {"int": [b"\n", b"+\n", b"2147483648\n", b"0x10\n"],
                 "real": [b"\n", b"NaN\n", b"0x1p2\n", b"1e-999\n", b"1e999\n"],
                 "bool": [b"True\n", b"1\n"], "text": []}
        for kind, values in cases.items():
            self.source.write_text(f"launch() {{ show(ask({kind})); }}")
            executable = self.compile()
            for data in [b"", *values]:
                with self.subTest(kind=kind, data=data):
                    result = self.execute(executable, data)
                    self.assertEqual(result.returncode, 1, result.stderr)
                    self.assertEqual(result.stdout, b"")
                    self.assertIn(b"error:", result.stderr)

    def test_text_lines_grow_preserve_spaces_and_validate_bytes(self):
        self.source.write_text('launch() { show("[" + ask(text) + "]"); }')
        executable = self.compile()
        long_line = ("বাংলা %s " * 20000).encode()
        for data, expected in [(long_line + b"\r\n", long_line), (b"  spaced  \n", b"  spaced  "),
                               (b"\n", b""), (b"last\r", b"last\r")]:
            with self.subTest(length=len(data)):
                result = self.execute(executable, data)
                self.assertEqual((result.returncode, result.stdout, result.stderr),
                                 (0, b"[" + expected + b"]\n", b""))
        for invalid in [b"\x00", b"a\x00b", b"\xff", b"\xed\xa0\x80", b"\xe2\x82"]:
            result = self.execute(executable, invalid + b"\n")
            self.assertEqual(result.returncode, 1, result.stderr)
            self.assertIn(b"invalid UTF-8 text", result.stderr)

    def test_file_wrappers_survive_rebinding_and_close_before_exit(self):
        result = self.run_observed(
            'launch() { vary output = fileOpen("note.txt", "write"); '
            'fileWriteTo(output, "first\\r\\n"); '
            'output = fileOpen("note.txt", "append"); fileWriteTo(output, "second"); '
            'fileClose(output); keep input = fileOpen("note.txt", "read"); '
            'keep contents = fileReadFrom(input); '
            'show(contents == fileRead("note.txt"), fileReadFrom(input) == ""); give; }',
            persistent_files=3, watch_files=True
        )
        self.assertEqual((result.returncode, result.stdout, result.stderr), (0, b"true true\n", b""))
        self.assertEqual((self.directory / "note.txt").read_bytes(), b"first\r\nsecond")

    def test_each_io_allocation_failure_closes_files_and_releases_buffers(self):
        data = b"x" * 5000 + b"\n +42 \n"
        result = self.run_observed(
            'launch() { keep line = ask(text); keep output = fileOpen("note.txt", "write"); '
            'fileWriteTo(output, line); keep input = fileOpen("note.txt", "read"); '
            'keep contents = fileReadFrom(input); keep copy = fileRead("note.txt"); '
            'keep number = toInt(ask(text)); keep words = [contents, copy, toText(number)]; '
            'walk word in words { keep saved = word + "!"; } }',
            persistent_files=2, input_data=data, watch_files=True
        )
        self.assertEqual((result.returncode, result.stdout, result.stderr), (0, b"", b""))
        calls = int((self.directory / "allocation-count.txt").read_text())
        self.assertGreater(calls, 20)
        for failure in range(1, calls + 1):
            with self.subTest(allocation=failure):
                result = self.run_observed_executable(failure, data)
                self.assertEqual(result.returncode, 1, result.stderr)
                self.assertIn(b"out of memory", result.stderr)

    def test_fatal_io_errors_close_live_and_internal_files(self):
        (self.directory / "invalid.txt").write_bytes(b"bad\x00text")
        programs = [
            ('launch() { keep output = fileOpen("out.txt", "write"); '
             'fileWriteTo(output, "flushed"); fileRead("invalid.txt"); }', b"invalid UTF-8"),
            ('launch() { keep output = fileOpen("out.txt", "write"); '
             'fileClose(output); fileClose(output); }', b"closed"),
            ('launch() { keep output = fileOpen("out.txt", "write"); '
             'keep value = toInt("not a number"); }', b"invalid integer"),
        ]
        for source, message in programs:
            result = self.run_observed(source, watch_files=True)
            self.assertEqual(result.returncode, 1, result.stderr)
            self.assertIn(message, result.stderr)

    def test_invalid_io_types_and_handle_aliases_fail_before_cc(self):
        programs = [
            'launch() { toInt(1); }', 'launch() { toReal(1.0); }',
            'launch() { toText("text"); }', 'launch() { toBool(true); }',
            'launch() { fileWrite(1, "text"); }', 'launch() { fileRead(true); }',
            'launch() { fileClose("handle"); }',
            'launch() { keep handle = fileOpen("x", "write"); keep copy = handle; }',
            'launch() { vary handle = fileOpen("x", "write"); handle = handle; }',
            'launch() { keep handle = fileOpen("x", "write"); '
            'handle = fileOpen("y", "write"); }',
        ]
        for source in programs:
            with self.subTest(source=source):
                self.source.write_text(source)
                result = self.invoke("input.ruv", environment=self.mock_compiler())
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(b"error:", result.stderr)
                self.assertFalse((self.directory / "arguments.json").exists())
                self.assert_no_temporary_outputs()

    def test_file_paths_use_process_directory_and_follow_regular_symlinks(self):
        name = "note বাংলা with spaces.txt"
        (self.directory / "link.txt").symlink_to(name)
        self.source.write_text(
            'launch() { keep path = ask(text); show(fileWrite(path, "bytes\\r\\n")); '
            'show(fileExists(path), fileExists("link.txt"), fileExists(".")); '
            'show(fileRead("link.txt")); }'
        )
        result = self.execute(self.compile(), name.encode() + b"\n")
        self.assertEqual((result.returncode, result.stdout, result.stderr),
                         (0, b"true\ntrue true false\nbytes\r\n\n", b""))
        self.assertEqual((self.directory / name).read_bytes(), b"bytes\r\n")


if __name__ == "__main__":
    unittest.main()
