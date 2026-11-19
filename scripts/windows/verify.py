#!/usr/bin/env python3

import argparse
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import time


ROOT = Path(__file__).resolve().parents[2]


def verification_commands(build):
    commands = []
    for suite in ("foundation", "lexer", "parser", "host"):
        commands.append([str(build / "tests" / suite / f"{suite}-tests.exe")])
    for suite, probe in (("foundation", "fixture-probe"), ("lexer", "lexer-probe"),
                         ("parser", "parser-probe"), ("semantic", "semantic-probe")):
        commands.append([
            sys.executable, "scripts/testing/fixtures.py", "--program",
            str(build / "tests" / suite / f"{probe}.exe"), f"tests/fixtures/{suite}",
        ])
    commands.append([
        sys.executable, "scripts/testing/fixtures.py", "--program",
        "scripts/testing/native.py", "tests/fixtures/native",
    ])
    commands.append([
        sys.executable, "-m", "unittest", "discover", "-s", "tests/integration",
        "-p", "test_modules.py", "-v",
    ])
    commands.append([sys.executable, "-m", "unittest", "discover", "-s", "tests/windows", "-v"])
    return commands


def main():
    parser = argparse.ArgumentParser(description="Verify Ruva on a real Windows x64 host.")
    parser.add_argument("--build-dir", type=Path, default=Path("build/windows"))
    arguments = parser.parse_args()
    if os.name != "nt":
        parser.error("Run this command on Windows using native Windows Python.")
    build = (ROOT / arguments.build_dir).resolve()
    compiler = build / "bin/ruvac.exe"
    if not compiler.is_file():
        parser.error("Build ruvac.exe first with make windows-build.")
    cc = shutil.which(os.environ.get("CC", "gcc"))
    if cc is None:
        parser.error("Put the UCRT64 GCC bin directory on PATH or set CC to gcc.exe.")
    target = subprocess.run([cc, "-dumpmachine"], capture_output=True, timeout=30)
    if target.returncode != 0 or target.stdout.strip() != b"x86_64-w64-mingw32":
        parser.error("Use the Windows x64 UCRT64 GCC toolchain.")
    environment = os.environ.copy()
    environment["CC"] = cc
    environment["RUVA_TEST_COMPILER"] = str(compiler)
    environment.pop("RUVA_TEST_SANITIZE", None)
    environment.pop("RUVA_RUNTIME_ROOT", None)
    environment["PYTHONUTF8"] = "1"
    report = {
        "started_utc": datetime.now(timezone.utc).isoformat(),
        "host": platform.platform(), "machine": platform.machine(),
        "python": sys.version, "compiler": str(compiler), "cc": cc, "checks": [],
        "target": target.stdout.decode("ascii").strip(),
        "excluded": ["POSIX-only descriptor/FIFO stress suites", "Windows GCC sanitizers"],
    }
    commands = [[cc, "--version"], [cc, "-dumpmachine"],
                [sys.executable, "scripts/repository/check-layout.py"]]
    commands.extend(verification_commands(build))
    failed = False
    with (build / "verification.log").open("wb") as log:
        for command in commands:
            print("Running:", subprocess.list2cmdline(command), flush=True)
            log.write(("\n$ " + subprocess.list2cmdline(command) + "\n").encode("utf-8"))
            started = time.monotonic()
            try:
                result = subprocess.run(command, cwd=ROOT, env=environment, capture_output=True,
                                        timeout=1200)
                log.write(result.stdout)
                log.write(result.stderr)
                returncode = result.returncode
            except (OSError, subprocess.TimeoutExpired) as error:
                log.write(str(error).encode("utf-8", errors="replace"))
                returncode = 1
            log.flush()
            report["checks"].append({
                "command": command, "exit": returncode,
                "seconds": round(time.monotonic() - started, 3),
            })
            failed = failed or returncode != 0
            print("FAIL" if returncode else "PASS", flush=True)
    report["passed"] = not failed
    (build / "verification.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"Results: {build / 'verification.log'}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
