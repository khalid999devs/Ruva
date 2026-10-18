import json
import os
from pathlib import Path
import sys
import tempfile
import unittest

from scripts.testing.fixtures import FixtureError, discover, run_case


class FixtureRunnerTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="ruva-runner-test-")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.manifest = self.root / "case.json"

    def write_case(self, script, **fields):
        case = {"args": ["-c", script], **fields}
        self.manifest.write_text(json.dumps(case), encoding="utf-8")
        return self.manifest

    def run_fixture(self):
        return run_case(self.manifest, sys.executable)

    def test_checks_binary_streams_exit_and_output_file(self):
        data = b"hello\x00\xff\r\n"
        (self.root / "input.bin").write_bytes(data)
        (self.root / "stderr.txt").write_bytes(b"checked\n")
        script = (
            "import sys; from pathlib import Path; data=sys.stdin.buffer.read(); "
            "sys.stdout.buffer.write(data); sys.stderr.write('checked\\n'); "
            "Path('result.bin').write_bytes(data); sys.exit(7)"
        )
        self.write_case(script, stdin="input.bin", stdout="input.bin", stderr="stderr.txt",
                        exit=7, files={"result.bin": "input.bin"})
        self.assertEqual(self.run_fixture(), [])

    def test_reports_deliberate_stdout_stderr_and_exit_mismatch(self):
        self.write_case("import sys; print('wrong'); sys.stderr.write('error'); sys.exit(2)")
        failures = self.run_fixture()
        for field in ("stdout:", "stderr:", "exit:"):
            self.assertTrue(any(field in failure for failure in failures), failures)

    def test_oracle_files_are_not_copied_into_workspace(self):
        (self.root / "result.txt").write_text("expected", encoding="utf-8")
        self.write_case("pass", files={"result.txt": "result.txt"})
        self.assertEqual(self.run_fixture(), ["file missing: result.txt"])

    def test_output_file_content_mismatch_is_reported(self):
        (self.root / "expected.txt").write_bytes(b"correct")
        self.write_case("from pathlib import Path; Path('out').write_bytes(b'wrong')",
                        files={"out": "expected.txt"})
        self.assertIn("file out:", self.run_fixture()[0])

    def test_each_run_gets_a_fresh_workspace(self):
        self.write_case("from pathlib import Path; p=Path('marker'); assert not p.exists(); p.touch()")
        self.assertEqual(self.run_fixture(), [])
        self.assertEqual(self.run_fixture(), [])
        self.assertFalse((self.root / "marker").exists())

    def test_nested_input_paths_and_spaces_are_preserved(self):
        directory = self.root / "sources"
        directory.mkdir()
        (directory / "name with spaces.ruv").write_bytes(b"unchanged\n")
        self.write_case(
            "from pathlib import Path; import sys; sys.stdout.buffer.write(Path(sys.argv[1]).read_bytes())",
            inputs=["sources/name with spaces.ruv"], stdout="sources/name with spaces.ruv",
        )
        case = json.loads(self.manifest.read_text())
        case["args"].append("sources/name with spaces.ruv")
        self.manifest.write_text(json.dumps(case))
        self.assertEqual(self.run_fixture(), [])

    def test_paths_cannot_leave_the_fixture(self):
        for fields in ({"stdin": "../outside"}, {"inputs": ["../outside"]},
                       {"files": {"../outside": "expected.txt"}}):
            with self.subTest(fields=fields):
                (self.root / "expected.txt").write_bytes(b"")
                self.write_case("pass", **fields)
                with self.assertRaises(FixtureError):
                    self.run_fixture()

    def test_timeout_is_a_failure(self):
        self.write_case("import time; time.sleep(1)", timeout=0.05)
        with self.assertRaisesRegex(FixtureError, "timed out"):
            self.run_fixture()

    def test_invalid_manifest_fields_are_rejected(self):
        for fields in ({"stdotu": "out"}, {"exit": True}, {"timeout": float("nan")},
                       {"timeout": 10 ** 1000}, {"timeout": 0}, {"args": ["bad\0argument"]},
                       {"args": "not an argument list"}):
            with self.subTest(fields=fields):
                self.write_case("pass", **fields)
                with self.assertRaises(FixtureError):
                    self.run_fixture()

    def test_missing_oracle_file_is_an_error(self):
        self.write_case("pass", stdout="missing.txt")
        with self.assertRaisesRegex(FixtureError, "missing"):
            self.run_fixture()

    def test_discovery_requires_real_cases(self):
        with self.assertRaisesRegex(FixtureError, "No case.json"):
            discover([self.root])
        self.write_case("pass")
        self.assertEqual(discover([self.root, self.manifest]), [self.manifest.resolve()])

    def test_discovery_collapses_relative_and_absolute_paths(self):
        self.write_case("pass")
        relative = Path(os.path.relpath(self.manifest, Path.cwd()))
        self.assertEqual(discover([self.root, relative]), [self.manifest.resolve()])


if __name__ == "__main__":
    unittest.main()
