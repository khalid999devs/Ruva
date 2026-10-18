#!/usr/bin/env python3

import argparse
from pathlib import Path
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[2]
IGNORE_RULES = ("/docs/", "AGENTS.md", ".agents/", ".codex/")
LOCAL_PATHS = (
    "docs", "docs/**", ":(glob)**/AGENTS.md",
    ":(glob)**/.agents/**", ":(glob)**/.codex/**",
)


def git(*arguments):
    result = subprocess.run(
        ["git", *arguments],
        cwd=ROOT,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    if result.returncode:
        message = result.stderr.decode("utf-8", errors="replace").strip()
        raise RuntimeError(message or "Git could not complete the document check.")
    return result.stdout


def check_index():
    ignore_rules = git("show", ":.gitignore").decode("utf-8").splitlines()
    for rule in IGNORE_RULES:
        if rule not in ignore_rules:
            raise RuntimeError(f"Keep {rule} in the staged .gitignore permanently.")

    if git("ls-files", "-z", "--", *LOCAL_PATHS):
        raise RuntimeError("docs/ and agent files are local only. Remove them from the index.")

    for path in (
        "docs/idea/.ignore-check", "docs/development/.ignore-check",
        "AGENTS.md", "nested/AGENTS.md", ".agents/.ignore-check", ".codex/.ignore-check",
    ):
        try:
            git("check-ignore", "--no-index", "--quiet", "--", path)
        except RuntimeError as error:
            raise RuntimeError("Keep local-only files ignored in the working tree as well.") from error


def check_history(revision):
    commit = git("rev-parse", "--verify", f"{revision}^{{commit}}").strip().decode("ascii")
    if git("rev-list", commit, "--", *LOCAL_PATHS):
        raise RuntimeError("Push blocked: local-only files appear in this revision's history.")
    ignore_rules = git("show", f"{commit}:.gitignore").decode("utf-8").splitlines()
    for rule in IGNORE_RULES:
        if rule not in ignore_rules:
            raise RuntimeError(f"Push blocked: the outgoing revision must ignore {rule}.")


def check_push_input():
    for line in sys.stdin:
        fields = line.split()
        if len(fields) != 4:
            raise RuntimeError("Invalid pre-push input; document protection was not bypassed.")
        local_ref, local_oid, _, _ = fields
        if local_oid and set(local_oid) == {"0"}:
            continue
        try:
            check_history(local_oid)
        except RuntimeError as error:
            raise RuntimeError(f"{local_ref}: {error}") from error


def main():
    parser = argparse.ArgumentParser(description="Keep local-only files out of Git.")
    modes = parser.add_mutually_exclusive_group()
    modes.add_argument("--history", metavar="REVISION")
    modes.add_argument("--pre-push", action="store_true")
    arguments = parser.parse_args()

    try:
        check_index()
        if arguments.history:
            check_history(arguments.history)
        elif arguments.pre_push:
            check_push_input()
    except (OSError, RuntimeError, UnicodeError) as error:
        print(f"Document check failed: {error}", file=sys.stderr)
        return 1

    print("Local-file check passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
