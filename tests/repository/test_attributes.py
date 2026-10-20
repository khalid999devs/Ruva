from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]


class FixtureAttributeTests(unittest.TestCase):
    def test_fixture_bytes_survive_git_line_ending_filters(self):
        samples = (
            b"// comment\r\nkeep ready = true;\r",
            b"\"a\0b\"",
            b"\"\xed\xa0\x80\"\n",
        )

        with tempfile.TemporaryDirectory(prefix="ruva-attributes-") as directory:
            root = Path(directory)
            shutil.copy2(ROOT / ".gitattributes", root / ".gitattributes")

            def git(*arguments, input_bytes=None):
                result = subprocess.run(
                    ["git", *arguments], cwd=root, input=input_bytes,
                    stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                )
                self.assertEqual(result.returncode, 0, result.stderr)
                return result.stdout

            git("init", "--quiet", "--initial-branch=main")
            path = "tests/fixtures/lexer/example/input.ruv"

            for mode in ("false", "true"):
                for sample in samples:
                    with self.subTest(autocrlf=mode, sample=sample):
                        expected = git("hash-object", "-w", "--stdin", input_bytes=sample)
                        actual = git(
                            "-c", f"core.autocrlf={mode}", "hash-object",
                            f"--path={path}", "--stdin", input_bytes=sample,
                        )
                        self.assertEqual(actual, expected)

                        restored = git(
                            "-c", f"core.autocrlf={mode}", "cat-file",
                            "--filters", f"--path={path}", expected.decode().strip(),
                        )
                        self.assertEqual(restored, sample)


if __name__ == "__main__":
    unittest.main()
