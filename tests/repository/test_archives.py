import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]


class ArchiveTests(unittest.TestCase):
    def test_source_removal_drops_old_archive_members(self):
        with tempfile.TemporaryDirectory(prefix="ruva-archive-") as temporary:
            directory = Path(temporary)
            makefile = directory / "Makefile"
            shutil.copy2(ROOT / "Makefile", makefile)
            (directory / "sample").mkdir()
            (directory / "sample/first.c").write_text("int first(void) { return 1; }\n")
            (directory / "sample/second.c").write_text("int second(void) { return 2; }\n")
            target = "build/check/lib/libruva_backend.a"

            def build(sources):
                result = subprocess.run(
                    ["make", "BUILD_DIR=build/check", f"BACKEND_SOURCES={sources}", target],
                    cwd=directory, capture_output=True,
                )
                self.assertEqual(result.returncode, 0, result.stderr)
                result = subprocess.run(["ar", "t", target], cwd=directory, capture_output=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                return [name for name in result.stdout.splitlines() if not name.startswith(b"__.SYMDEF")]

            self.assertEqual(build("sample/first.c sample/second.c"), [b"first.o", b"second.o"])
            timestamp = (directory / target).stat().st_mtime + 2
            makefile.write_text(makefile.read_text() + "\n")
            os.utime(makefile, (timestamp, timestamp))
            self.assertEqual(build("sample/first.c"), [b"first.o"])


if __name__ == "__main__":
    unittest.main()
