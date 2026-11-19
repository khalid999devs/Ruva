#!/usr/bin/env python3

import argparse
import hashlib
from pathlib import Path
import subprocess
import sys
import zipfile


ROOT = Path(__file__).resolve().parents[2]
LOCAL_NAMES = {"agents.md", ".agents", ".codex", ".git"}
GENERATED_ROOTS = {"build", "bin", "out", "dist", "coverage"}
WINDOWS_DEVICES = {"con", "prn", "aux", "nul"}
WINDOWS_DEVICES.update(f"{prefix}{number}" for prefix in ("com", "lpt") for number in range(1, 10))


def check_windows_path(relative):
    for part in relative.parts:
        stem = part.split(".", 1)[0].casefold()
        if (stem in WINDOWS_DEVICES or part.endswith((" ", "."))
            or any(character in part for character in '<>:"\\|?*')):
            raise ValueError(f"Path cannot be extracted safely on Windows: {relative}")


def public_files(root):
    result = subprocess.run(
        ["git", "ls-files", "--cached", "--others", "--exclude-standard", "-z"],
        cwd=root, check=True, stdout=subprocess.PIPE,
    )
    paths = sorted(set(Path(name.decode("utf-8")) for name in result.stdout.split(b"\0") if name))
    files = []
    for relative in paths:
        check_windows_path(relative)
        parts = [part.casefold() for part in relative.parts]
        if (relative.is_absolute() or ".." in parts or parts[0] == "docs"
            or parts[0] in GENERATED_ROOTS or any(part in LOCAL_NAMES for part in parts)):
            raise ValueError(f"Refusing a local-only or generated path: {relative}")
        source = root / relative
        if source.is_symlink():
            raise ValueError(f"Refusing a symlink in the source package: {relative}")
        if source.is_file():
            source.resolve().relative_to(root.resolve())
            files.append(relative)
    return files


def create_archive(root, output):
    files = public_files(root)
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_suffix(".zip.tmp")
    try:
        with zipfile.ZipFile(temporary, "w", compression=zipfile.ZIP_DEFLATED) as archive:
            for relative in files:
                archive.write(root / relative, (Path("Ruva") / relative).as_posix())
        temporary.replace(output)
    finally:
        temporary.unlink(missing_ok=True)
    digest = hashlib.sha256(output.read_bytes()).hexdigest()
    output.with_suffix(".zip.sha256").write_text(f"{digest}  {output.name}\n", encoding="ascii")
    return len(files)


def main():
    parser = argparse.ArgumentParser(description="Package the public working sources for Windows testing.")
    parser.add_argument("--output", type=Path, default=Path("build/windows/ruva-windows-source.zip"))
    arguments = parser.parse_args()
    output = (ROOT / arguments.output).resolve()
    try:
        output.relative_to(ROOT / "build")
        if output.suffix != ".zip":
            raise ValueError("The package output must be a .zip file under build/.")
        count = create_archive(ROOT, output)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        print(f"Cannot package sources: {error}", file=sys.stderr)
        return 1
    print(f"Packaged {count} public source files: {output}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
