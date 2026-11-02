#!/usr/bin/env python3
"""Format maintained desktop sources, or check them without changing files."""

import argparse
import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
LOCAL_TOOLS = ROOT / "build/format-tools/bin"
SOURCE_DIRECTORIES = ("src", "include", "assembler", "studio", "tests", "tools")


def executable(name, override):
    candidate = override or (LOCAL_TOOLS / name if (LOCAL_TOOLS / name).is_file() else name)
    resolved = shutil.which(str(candidate))
    if not resolved:
        raise ValueError(f"Cannot find {candidate}. See docs/CODING_STYLE.md for formatter setup.")
    return resolved


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--clang-format", default=os.environ.get("CLANG_FORMAT"))
    parser.add_argument("--black", default=os.environ.get("BLACK"))
    arguments = parser.parse_args()
    try:
        clang_format = executable("clang-format", arguments.clang_format)
        black = executable("black", arguments.black)
        version = subprocess.check_output([clang_format, "--version"], text=True)
        if "version 18.1.8" not in version:
            raise ValueError("Use clang-format 18.1.8 to keep formatting reproducible.")
        files = sorted(
            path
            for directory in SOURCE_DIRECTORIES
            for path in (ROOT / directory).rglob("*")
            if path.is_file()
        )
        native = [str(path) for path in files if path.suffix in {".c", ".cpp", ".h"}]
        options = ["--dry-run", "--Werror"] if arguments.check else ["-i"]
        subprocess.run([clang_format, *options, *native], cwd=ROOT, check=True)
        options = ["--check", "--quiet"] if arguments.check else ["--quiet"]
        # Serial calls also work in environments that disallow worker sockets.
        for path in files:
            if path.suffix == ".py":
                subprocess.run([black, *options, str(path)], cwd=ROOT, check=True)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        parser.exit(1, f"Source formatting failed: {error}\n")
    print("Desktop source formatting checked." if arguments.check else "Desktop sources formatted.")


if __name__ == "__main__":
    main()
