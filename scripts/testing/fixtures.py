#!/usr/bin/env python3

import argparse
import json
import math
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


class FixtureError(Exception):
    pass


def inside(root, name):
    if not isinstance(name, str) or not name or "\0" in name or Path(name).is_absolute():
        raise FixtureError("Fixture paths must be nonempty relative paths.")
    path = (root / name).resolve()
    try:
        path.relative_to(root.resolve())
    except ValueError as error:
        raise FixtureError(f"Path leaves the fixture directory: {name}") from error
    return path


def read_expected(root, name):
    path = inside(root, name)
    if not path.is_file():
        raise FixtureError(f"Expected fixture file is missing: {name}")
    return path.read_bytes()


def string_list(value, field):
    if not isinstance(value, list) or any(not isinstance(item, str) for item in value):
        raise FixtureError(f"{field} must be a list of strings.")
    if any("\0" in item for item in value):
        raise FixtureError(f"{field} must not contain NUL bytes.")
    return value


def difference(label, expected, actual):
    return f"{label}: expected {expected[:120]!r}, got {actual[:120]!r}"


def run_case(manifest_path, program):
    manifest_path = Path(manifest_path).resolve()
    root = manifest_path.parent
    try:
        case = json.loads(manifest_path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, ValueError) as error:
        raise FixtureError(f"Cannot read fixture: {error}") from error
    if not isinstance(case, dict):
        raise FixtureError("A fixture manifest must be a JSON object.")
    allowed = {"args", "inputs", "stdin", "stdout", "stderr", "exit", "files", "timeout"}
    if case.keys() - allowed:
        raise FixtureError(f"Unknown fixture fields: {', '.join(sorted(case.keys() - allowed))}")
    arguments = string_list(case.get("args", []), "args")
    inputs = string_list(case.get("inputs", []), "inputs")
    expected_exit = case.get("exit", 0)
    if type(expected_exit) is not int:
        raise FixtureError("exit must be an integer.")
    timeout = case.get("timeout", 5)
    if type(timeout) not in (int, float):
        raise FixtureError("timeout must be a positive finite number.")
    try:
        timeout = float(timeout)
    except OverflowError as error:
        raise FixtureError("timeout must be a positive finite number.") from error
    if not math.isfinite(timeout) or timeout <= 0:
        raise FixtureError("timeout must be a positive finite number.")
    files = case.get("files", {})
    if not isinstance(files, dict):
        raise FixtureError("files must map output paths to expected fixture files.")
    stdin = read_expected(root, case["stdin"]) if "stdin" in case else b""
    stdout = read_expected(root, case["stdout"]) if "stdout" in case else b""
    stderr = read_expected(root, case["stderr"]) if "stderr" in case else b""
    outputs = {name: read_expected(root, expected) for name, expected in files.items()}
    sources = [(name, inside(root, name)) for name in inputs]
    if any(not source.is_file() for _, source in sources):
        raise FixtureError("Every input must name an existing fixture file.")
    executable = Path(program).resolve()
    if not executable.is_file():
        raise FixtureError(f"Program does not exist: {executable}")

    with tempfile.TemporaryDirectory(prefix="ruva-fixture-") as temporary:
        workspace = Path(temporary)
        for name in outputs:
            inside(workspace, name)
        for name, source in sources:
            target = inside(workspace, name)
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, target)
        try:
            result = subprocess.run(
                [str(executable), *arguments], cwd=workspace, input=stdin,
                stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=timeout,
            )
        except subprocess.TimeoutExpired as error:
            raise FixtureError(f"Program timed out after {timeout:g} seconds.") from error
        except OSError as error:
            raise FixtureError(f"Cannot run program: {error}") from error

        failures = []
        if result.returncode != expected_exit:
            failures.append(f"exit: expected {expected_exit}, got {result.returncode}")
        if result.stdout != stdout:
            failures.append(difference("stdout", stdout, result.stdout))
        if result.stderr != stderr:
            failures.append(difference("stderr", stderr, result.stderr))
        for name, expected in outputs.items():
            output = inside(workspace, name)
            if not output.is_file():
                failures.append(f"file missing: {name}")
            else:
                actual = output.read_bytes()
                if actual != expected:
                    failures.append(difference(f"file {name}", expected, actual))
        return failures


def discover(paths):
    manifests = set()
    for value in paths:
        path = Path(value)
        if path.is_dir():
            manifests.update(manifest.resolve() for manifest in path.rglob("case.json"))
        elif path.is_file() and path.name == "case.json":
            manifests.add(path.resolve())
        else:
            raise FixtureError(f"Not a fixture directory or case.json: {path}")
    if not manifests:
        raise FixtureError("No case.json fixtures found.")
    return sorted(manifests)


def main():
    parser = argparse.ArgumentParser(description="Run isolated byte-exact program fixtures.")
    parser.add_argument("--program", required=True, type=Path)
    parser.add_argument("paths", nargs="+", type=Path)
    arguments = parser.parse_args()
    try:
        manifests = discover(arguments.paths)
    except FixtureError as error:
        print(error, file=sys.stderr)
        return 1
    failed = 0
    for manifest in manifests:
        try:
            failures = run_case(manifest, arguments.program)
        except (FixtureError, OSError, RuntimeError) as error:
            failures = [str(error)]
        label = manifest.parent
        if failures:
            failed += 1
            print(f"FAIL {label}", file=sys.stderr)
            for failure in failures:
                print(f"  {failure}", file=sys.stderr)
        else:
            print(f"PASS {label}")
    print(f"{len(manifests) - failed}/{len(manifests)} fixtures passed.")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
