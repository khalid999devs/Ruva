import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
COMPILER = Path(os.environ.get("RUVA_TEST_COMPILER", ROOT / "build/windows/bin/ruvac.exe")).resolve()


class WindowsCompilerTestCase(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tools = tempfile.TemporaryDirectory(prefix="ruva-windows-tools-")
        cls.addClassCleanup(cls.tools.cleanup)
        cls.mock = Path(cls.tools.name) / "compiler বাংলা with spaces.exe"
        result = subprocess.run(
            [os.environ.get("CC", "gcc"), "-std=c11", "-Wall", "-Wextra", "-Werror", "-municode",
             str(ROOT / "tests/windows/process.c"), "-o", str(cls.mock)],
            capture_output=True, timeout=60,
        )
        if result.returncode:
            raise RuntimeError(result.stderr.decode("utf-8", errors="replace"))

    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="ruva-windows-")
        self.addCleanup(self.temporary.cleanup)
        self.directory = Path(self.temporary.name)
        self.source = self.directory / "input.ruv"
        self.source.write_bytes(b'launch() { show("native"); }\n')
        self.environment = os.environ.copy()
        self.environment["TMPDIR"] = str(self.directory)
        self.environment.pop("RUVA_RUNTIME_ROOT", None)

    def invoke(self, *arguments, environment=None):
        return subprocess.run([str(COMPILER), *map(str, arguments)], cwd=self.directory,
                              env=environment or self.environment, capture_output=True, timeout=90)

    def compile(self, source=None, output="program.exe"):
        result = self.invoke(source or self.source, "-o", output)
        self.assertEqual((result.returncode, result.stdout, result.stderr), (0, b"", b""))
        self.assert_clean()
        return self.directory / output

    def execute(self, executable, data=b""):
        return subprocess.run([str(executable)], cwd=self.directory, input=data,
                              capture_output=True, timeout=30)

    def mock_environment(self, mode="failure"):
        environment = self.environment.copy()
        environment["CC"] = str(self.mock)
        environment["RV_MOCK_MODE"] = mode
        return environment

    def assert_clean(self):
        self.assertFalse(list(self.directory.rglob(".ruva-output-*")))
        self.assertFalse(list(self.directory.glob("ruvac-*")))

    def copy_compiler(self, name):
        destination = self.directory / name
        shutil.copy2(COMPILER, destination)
        return destination
