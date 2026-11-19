import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
import zipfile

from scripts.testing.fixtures import run_case
from scripts.windows.package import check_windows_path, create_archive, public_files
from scripts.windows.verify import verification_commands


class WindowsPreparationTests(unittest.TestCase):
    def test_windows_package_rejects_reserved_device_names_and_path_aliases(self):
        for name in ("tests/nul/input.ruv", "CON.txt", "aux", "com1.c", "folder/trailing.",
                     "folder/trailing ", "folder/stream:name", "folder/bad?.ruv"):
            with self.subTest(name=name), self.assertRaises(ValueError):
                check_windows_path(Path(name))
        check_windows_path(Path("tests/fixtures/lexer/embedded-nul/input.ruv"))

    def test_python_fixture_uses_current_interpreter_without_executable_permission(self):
        with tempfile.TemporaryDirectory(prefix="ruva-python-fixture-") as temporary:
            root = Path(temporary)
            script = root / "tool with spaces.py"
            script.write_text("import sys\nsys.stdout.buffer.write(b'exact\\r\\n')\n", encoding="utf-8")
            script.chmod(0o600)
            (root / "expected.txt").write_bytes(b"exact\r\n")
            manifest = root / "case.json"
            manifest.write_text(json.dumps({"stdout": "expected.txt"}), encoding="utf-8")
            self.assertEqual(run_case(manifest, script), [])

    def test_windows_matrix_contains_compiler_runtime_and_fixture_checks(self):
        commands = verification_commands(Path("build/windows"))
        suites = [Path(command[0]).name for command in commands[:4]]
        self.assertEqual(suites, ["foundation-tests.exe", "lexer-tests.exe", "parser-tests.exe", "host-tests.exe"])
        fixtures = [command[-1] for command in commands if "scripts/testing/fixtures.py" in command]
        self.assertEqual(fixtures, ["tests/fixtures/foundation", "tests/fixtures/lexer",
                                    "tests/fixtures/parser", "tests/fixtures/semantic", "tests/fixtures/native"])
        self.assertIn("tests/windows", commands[-1])
        self.assertIn("tests/integration", commands[-2])
        self.assertIn("test_modules.py", commands[-2])
        self.assertTrue(all(command[0] == sys.executable for command in commands[4:]))

    def test_source_package_preserves_bytes_and_excludes_ignored_local_files(self):
        with tempfile.TemporaryDirectory(prefix="ruva-source-package-") as temporary:
            root = Path(temporary)
            subprocess.run(["git", "init", "--quiet", str(root)], check=True)
            (root / ".gitignore").write_text("/docs/\nAGENTS.md\n.agents/\n.codex/\n/build/\n", encoding="utf-8")
            (root / "docs").mkdir()
            (root / "docs/private.md").write_text("local only", encoding="utf-8")
            (root / "AGENTS.md").write_text("local only", encoding="utf-8")
            (root / "source.ruv").write_bytes(b'launch() {}\r\n\x00\xff')
            output = root / "build/source.zip"
            self.assertEqual(create_archive(root, output), 2)
            with zipfile.ZipFile(output) as archive:
                self.assertEqual(set(archive.namelist()), {"Ruva/.gitignore", "Ruva/source.ruv"})
                self.assertEqual(archive.read("Ruva/source.ruv"), (root / "source.ruv").read_bytes())
            self.assertTrue(output.with_suffix(".zip.sha256").is_file())
            self.assertEqual(public_files(root), [Path(".gitignore"), Path("source.ruv")])

    def test_source_package_refuses_unignored_private_paths_before_writing(self):
        with tempfile.TemporaryDirectory(prefix="ruva-source-refusal-") as temporary:
            root = Path(temporary)
            subprocess.run(["git", "init", "--quiet", str(root)], check=True)
            (root / "AGENTS.md").write_text("local only", encoding="utf-8")
            output = root / "build/source.zip"
            with self.assertRaisesRegex(ValueError, "Refusing"):
                create_archive(root, output)
            self.assertFalse(output.exists())


if __name__ == "__main__":
    unittest.main()
