#!/usr/bin/env python3

import os
from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[2]
FILE_LIMIT = 6
GENERATED_DIRECTORIES = {
    ".git", ".cache", ".venv", "venv", "__pycache__", ".pytest_cache",
    ".mypy_cache", ".ruff_cache", ".idea", ".vscode",
}
ROOT_OUTPUT_DIRECTORIES = {"build", "bin", "out", "dist", "coverage"}
ROOT_METADATA_FILES = {"LICENSE"}
SYSTEM_FILES = {".DS_Store", "Thumbs.db"}


def find_overfull_directories():
    violations = []
    for directory, children, files in os.walk(ROOT, followlinks=False):
        current = Path(directory)
        excluded = GENERATED_DIRECTORIES.copy()
        skipped = SYSTEM_FILES.copy()
        if current == ROOT:
            excluded.update(ROOT_OUTPUT_DIRECTORIES)
            skipped.update(ROOT_METADATA_FILES)
        children[:] = sorted(name for name in children if name not in excluded)
        project_files = sorted(name for name in files if name not in skipped)
        if len(project_files) > FILE_LIMIT:
            location = current.relative_to(ROOT).as_posix()
            violations.append((location, len(project_files)))
    return violations


def main():
    violations = find_overfull_directories()
    if violations:
        for directory, count in violations:
            print(
                f"Layout check failed: {directory} has {count} files; "
                f"group them into subdirectories (limit {FILE_LIMIT}).",
                file=sys.stderr,
            )
        return 1
    print(f"Layout check passed: at most {FILE_LIMIT} project files per directory.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
