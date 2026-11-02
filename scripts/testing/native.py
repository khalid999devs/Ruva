#!/usr/bin/env python3

import os
from pathlib import Path
import subprocess
import sys


def main():
    compiler = os.environ.get("RUVA_TEST_COMPILER")
    if compiler is None or len(sys.argv) != 2:
        print("native fixture requires a compiler and one source", file=sys.stderr)
        return 1
    output = "program.exe" if os.name == "nt" else "program"
    arguments = [compiler, sys.argv[1], "-o", output]
    if os.environ.get("RUVA_TEST_SANITIZE") == "1":
        arguments.append("--sanitize")
    result = subprocess.run(arguments, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if result.returncode == 0:
        result = subprocess.run([str(Path(output).resolve())],
                                stdin=sys.stdin.buffer, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    sys.stdout.buffer.write(result.stdout)
    prefix = Path.cwd().resolve().as_posix().encode("utf-8") + b"/"
    sys.stderr.buffer.write(result.stderr.replace(prefix, b""))
    return result.returncode


if __name__ == "__main__":
    sys.exit(main())
