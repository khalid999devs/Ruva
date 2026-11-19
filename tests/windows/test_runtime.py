import os
import unittest

from support import WindowsCompilerTestCase


@unittest.skipUnless(os.name == "nt", "requires real Windows")
class WindowsRuntimeTests(WindowsCompilerTestCase):
    def test_utf8_files_and_exact_line_endings(self):
        name = "note বাংলা & spaces.txt"
        self.source.write_bytes(
            b'launch() { keep path = ask(text); show(fileWrite(path, "first\\r\\n")); '
            b'keep output = fileOpen(path, "append"); fileWriteTo(output, "second\\n"); '
            b'fileClose(output); show(fileExists(path), fileExists(".")); '
            b'show(fileRead(path)); }'
        )
        result = self.execute(self.compile(), name.encode("utf-8") + b"\r\n")
        self.assertEqual((result.returncode, result.stdout, result.stderr),
                         (0, b"true\ntrue false\nfirst\r\nsecond\n\n", b""))
        self.assertEqual((self.directory / name).read_bytes(), b"first\r\nsecond\n")

    def test_console_bytes_keep_bare_cr_ctrl_z_and_utf8(self):
        self.source.write_bytes(b'launch() { show("[" + ask(text) + "]"); }')
        executable = self.compile()
        for data, expected in ((b"last\r", b"last\r"), (b"before\x1aafter\n", b"before\x1aafter"),
                               (" বাংলা \r\n".encode("utf-8"), " বাংলা ".encode("utf-8"))):
            with self.subTest(data=data):
                result = self.execute(executable, data)
                self.assertEqual((result.returncode, result.stdout, result.stderr),
                                 (0, b"[" + expected + b"]\n", b""))

    def test_closed_handles_and_invalid_external_text_fail_cleanly(self):
        self.source.write_bytes(
            b'launch() { keep output = fileOpen("output.txt", "write"); '
            b'fileClose(output); fileWriteTo(output, "bad"); }'
        )
        result = self.execute(self.compile())
        self.assertEqual(result.returncode, 1)
        self.assertIn(b"file handle is closed", result.stderr)
        self.assertEqual((self.directory / "output.txt").read_bytes(), b"")
        self.source.write_bytes(b'launch() { show(ask(text)); }')
        executable = self.compile()
        for invalid in (b"\xff\n", b"a\0b\n"):
            result = self.execute(executable, invalid)
            self.assertEqual(result.returncode, 1)
            self.assertIn(b"invalid UTF-8 text", result.stderr)

    def test_numeric_conversion_and_formatting_match_other_hosts(self):
        self.source.write_bytes(
            b'launch() { show(toReal(" +.125e2 "), toInt("-2147483648"), '
            b'toText(0.1), toBool(" true "), toText(-0.0)); }'
        )
        result = self.execute(self.compile())
        self.assertEqual((result.returncode, result.stdout, result.stderr),
                         (0, b"12.5 -2147483648 0.10000000000000001 true -0\n", b""))
