import os
from pathlib import Path
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
COMPILER = Path(os.environ.get("RUVA_TEST_COMPILER", ROOT / "build/debug/bin/ruvac")).resolve()
SANITIZE = os.environ.get("RUVA_TEST_SANITIZE") == "1"
RUNTIME_SOURCES = [
    "core/runtime.c", "core/memory.c", "core/numeric.c",
    "text/text.c", "text/format.c", "text/output.c",
    "text/snapshot.c", "collections/array.c",
    "conversion/parse.c", "conversion/numeric.c", "io/read.c", "io/input.c",
    "files/handle.c", "files/path.c",
    "../compiler/common/host/files.c", "../compiler/common/host/numeric.c",
    "../compiler/common/host/streams.c",
]
if os.name == "nt":
    RUNTIME_SOURCES.append("../compiler/common/host/win32/utf8.c")


class CompilerTestCase(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="ruva-driver-")
        self.addCleanup(self.temporary.cleanup)
        self.directory = Path(self.temporary.name)
        self.source = self.directory / "input.ruv"
        self.source.write_text('launch() { show("native"); }\n')
        self.environment = os.environ.copy()
        self.environment.pop("RUVA_RUNTIME_ROOT", None)
        self.environment["TMPDIR"] = str(self.directory)

    def invoke(self, *arguments, environment=None):
        return subprocess.run([str(COMPILER), *map(str, arguments)], cwd=self.directory,
                              env=environment or self.environment, capture_output=True, timeout=30)

    def compile(self, source="input.ruv", output="program"):
        arguments = [source, "-o", output]
        if SANITIZE:
            arguments.append("--sanitize")
        result = self.invoke(*arguments)
        self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
        return self.directory / output

    def mock_compiler(self, exit_code=7, signal=False):
        path = self.directory / ("compiler with spaces " + ("signal" if signal else str(exit_code)))
        path.write_text(
            "#!/usr/bin/env python3\n"
            "import json, os, signal, sys\n"
            "from pathlib import Path\n"
            "Path('arguments.json').write_text(json.dumps(sys.argv[1:]))\n"
            + ("os.kill(os.getpid(), signal.SIGTERM)\n" if signal else f"sys.exit({exit_code})\n")
        )
        path.chmod(0o700)
        environment = self.environment.copy()
        environment["CC"] = str(path)
        return environment

    def assert_no_temporary_outputs(self):
        self.assertFalse(list(self.directory.glob(".ruva-output-*")))
        self.assertFalse(list(self.directory.glob("ruvac-*")))

