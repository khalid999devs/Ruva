from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]


class RepositoryGuardTests(unittest.TestCase):
    def setUp(self):
        self.workspace = tempfile.TemporaryDirectory(prefix="ruva-guards-")
        self.addCleanup(self.workspace.cleanup)
        self.root = Path(self.workspace.name)
        for name in (".gitignore", "README.md"):
            shutil.copy2(ROOT / name, self.root / name)
        for name in (".githooks", "scripts"):
            shutil.copytree(ROOT / name, self.root / name)
        for hook in (self.root / ".githooks").iterdir():
            hook.chmod(0o755)
        self.git("init", "--quiet", "--initial-branch=main")
        for key, value in (
            ("user.name", "Repository Guard Tests"),
            ("user.email", "tests@example.invalid"),
            ("commit.gpgSign", "false"),
            ("core.hooksPath", ".githooks"),
        ):
            self.git("config", key, value)
        self.git("add", ".gitignore", "README.md", ".githooks", "scripts")

    def run_command(self, *arguments, input_text=None):
        return subprocess.run(
            arguments,
            cwd=self.root,
            input=input_text,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )

    def git(self, *arguments):
        result = self.run_command("git", *arguments)
        self.assertEqual(result.returncode, 0, result.stderr)
        return result.stdout.strip()

    def guard(self, *arguments):
        return self.run_command("python3", "scripts/repository/check-docs.py", *arguments)

    def layout(self):
        return self.run_command("python3", "scripts/repository/check-layout.py")

    def add_local_document(self):
        path = self.root / "docs" / "development" / "private.md"
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text("Synthetic local-only test fixture.\n", encoding="utf-8")
        return path

    def push_hook(self, oid):
        update = f"refs/heads/main {oid} refs/heads/main {'0' * 40}\n"
        return self.run_command(".githooks/pre-push", input_text=update)

    def test_clean_index_passes(self):
        result = self.guard()
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_normal_add_keeps_documents_ignored(self):
        self.add_local_document()
        self.git("add", "--all")
        self.assertEqual(self.git("ls-files", "--", "docs"), "")
        self.assertEqual(self.guard().returncode, 0)

    def test_commit_hook_rejects_force_added_document(self):
        self.add_local_document()
        self.git("add", "--force", "docs/development/private.md")
        result = self.run_command("git", "commit", "-m", "Rejected document fixture")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("local only", result.stderr)

    def test_staged_ignore_rule_cannot_be_removed(self):
        (self.root / ".gitignore").write_text("/build/\n", encoding="utf-8")
        self.git("add", ".gitignore")
        result = self.guard()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("staged .gitignore", result.stderr)

    def test_working_tree_must_also_ignore_documents(self):
        (self.root / ".gitignore").write_text("/build/\n", encoding="utf-8")
        result = self.guard()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("working tree", result.stderr)

    def test_agent_files_are_ignored(self):
        for path in ("AGENTS.md", "nested/AGENTS.md", ".agents/local.md", ".codex/config.toml"):
            with self.subTest(path=path):
                target = self.root / path
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_text("Synthetic local fixture.\n", encoding="utf-8")
                self.git("check-ignore", "--quiet", "--", path)

    def test_agent_files_cannot_be_force_added(self):
        for path in ("AGENTS.md", "nested/AGENTS.md", ".agents/local.md", ".codex/config.toml"):
            with self.subTest(path=path):
                target = self.root / path
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_text("Synthetic local fixture.\n", encoding="utf-8")
                self.git("add", "--force", "--", path)
                result = self.guard()
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("local only", result.stderr)
                self.git("rm", "--cached", "--", path)

    def test_clean_push_passes(self):
        self.git("commit", "--quiet", "-m", "Clean fixture")
        result = self.push_hook(self.git("rev-parse", "HEAD"))
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_push_rejects_documents_even_after_they_are_deleted(self):
        self.git("commit", "--quiet", "-m", "Clean fixture")
        path = self.add_local_document()
        self.git("add", "--force", "docs/development/private.md")
        self.git(
            "-c", "core.hooksPath=/dev/null", "commit", "--quiet",
            "-m", "Synthetic forbidden history fixture",
        )
        path.unlink()
        self.git("add", "--update", "--", "docs")
        self.git("commit", "--quiet", "-m", "Remove synthetic fixture")
        self.assertEqual(self.git("ls-files", "--", "docs"), "")
        result = self.push_hook(self.git("rev-parse", "HEAD"))
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("history", result.stderr)

    def test_push_deletion_does_not_require_a_local_commit(self):
        result = self.push_hook("0" * 40)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_six_files_are_allowed(self):
        directory = self.root / "compiler" / "driver"
        directory.mkdir(parents=True)
        for index in range(6):
            (directory / f"file_{index}.c").touch()
        result = self.layout()
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_seventh_file_is_rejected_even_in_local_docs(self):
        directory = self.root / "docs" / "development" / "fixture"
        directory.mkdir(parents=True)
        for index in range(7):
            (directory / f"file_{index}.md").touch()
        result = self.layout()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("docs/development/fixture has 7 files", result.stderr)

    def test_root_license_is_not_counted(self):
        for index in range(4):
            (self.root / f"file_{index}.txt").touch()
        (self.root / "LICENSE").touch()
        result = self.layout()
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_license_outside_root_still_counts(self):
        directory = self.root / "compiler" / "driver"
        directory.mkdir(parents=True)
        for index in range(6):
            (directory / f"file_{index}.c").touch()
        (directory / "LICENSE").touch()
        result = self.layout()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("compiler/driver has 7 files", result.stderr)

    def test_generated_output_is_excluded(self):
        directory = self.root / "build"
        directory.mkdir()
        for index in range(8):
            (directory / f"file_{index}.o").touch()
        result = self.layout()
        self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == "__main__":
    unittest.main()
