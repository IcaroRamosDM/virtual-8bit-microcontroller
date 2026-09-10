#!/usr/bin/env python3
"""Build VM8 Studio through Make. Python is needed only on the build host."""

import argparse
import hashlib
import json
import fcntl
from pathlib import Path
import shlex
import subprocess
import tarfile
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
DEPS = ROOT / "build/studio-deps"
VERSION = "1.4.5"
ARCHIVE_SHA256 = "eede1fb2b8e9c2e581e77082e15252145855c79aad30070ee3b24aabe2f926f1"
URL = f"https://github.com/fltk/fltk/releases/download/release-{VERSION}/fltk-{VERSION}-source.tar.gz"
C_SOURCES = [
    "src/cpu.c", "src/byte_value.c", "src/instruction_set.c",
    "assembler/assembly.c", "assembler/byte_literal.c", "assembler/byte_operand.c",
    "assembler/first_pass.c", "assembler/instruction_encoder.c",
    "assembler/instruction_parser.c", "assembler/second_pass.c",
    "assembler/source_line.c", "assembler/symbol_table.c",
]
WINDOWS_FLAGS = ["-D__USE_MINGW_ANSI_STDIO=1", "-fno-builtin-printf",
                 "-fno-builtin-fprintf", "-fno-builtin-snprintf"]


def run(arguments, **kwargs):
    subprocess.run([str(value) for value in arguments], cwd=ROOT, check=True, **kwargs)


def dependencies(platform):
    DEPS.mkdir(parents=True, exist_ok=True)
    # Make may request both platforms in parallel. Keep extraction and install atomic.
    with (DEPS / "build.lock").open("w") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        return build_dependencies(platform)


def build_dependencies(platform):
    DEPS.mkdir(parents=True, exist_ok=True)
    archive = DEPS / f"fltk-{VERSION}-source.tar.gz"
    if not archive.exists():
        print(f"Downloading FLTK {VERSION} from the official release", flush=True)
        urllib.request.urlretrieve(URL, archive)
    if hashlib.sha256(archive.read_bytes()).hexdigest() != ARCHIVE_SHA256:
        raise RuntimeError("FLTK checksum mismatch; no archive was extracted")
    source = DEPS / f"fltk-{VERSION}"
    if not source.exists():
        with tarfile.open(archive) as package:
            # Support Python 3.11 on the Debian 12 compatibility baseline.
            # The pinned source archive needs only directories and regular files.
            for member in package:
                if (member.name.startswith("/") or ".." in Path(member.name).parts
                        or not (member.isdir() or member.isfile())):
                    raise RuntimeError("Unsafe FLTK source archive member")
            package.extractall(DEPS)
    build = DEPS / f"build-{platform}"
    prefix = DEPS / f"install-{platform}"
    marker = prefix / ".vm8-fltk-version"
    if marker.exists() and marker.read_text() == VERSION:
        return prefix
    options = [
        "cmake", "-S", source, "-B", build,
        f"-DCMAKE_INSTALL_PREFIX={prefix}", "-DCMAKE_INSTALL_LIBDIR=lib",
        "-DCMAKE_BUILD_TYPE=Release", "-DFLTK_BUILD_SHARED_LIBS=OFF",
        "-DFLTK_BUILD_TEST=OFF", "-DFLTK_BUILD_EXAMPLES=OFF", "-DFLTK_BUILD_GL=OFF",
        "-DFLTK_BUILD_FLUID=OFF", "-DFLTK_BUILD_FLTK_OPTIONS=OFF", "-DFLTK_BUILD_FORMS=OFF",
        "-DFLTK_BUILD_HTML_DOCS=OFF", "-DFLTK_BUILD_PDF_DOCS=OFF",
        "-DFLTK_OPTION_PRINT_SUPPORT=OFF", "-DFLTK_OPTION_SVG=OFF",
        "-DFLTK_USE_SYSTEM_LIBPNG=OFF", "-DFLTK_USE_SYSTEM_LIBJPEG=OFF",
        "-DFLTK_USE_SYSTEM_ZLIB=OFF", "-DFLTK_BACKEND_WAYLAND=OFF",
    ]
    if platform == "windows":
        options += ["-DCMAKE_SYSTEM_NAME=Windows", "-DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc",
                    "-DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++",
                    "-DCMAKE_RC_COMPILER=x86_64-w64-mingw32-windres",
                    "-DCMAKE_C_FLAGS=" + " ".join(WINDOWS_FLAGS),
                    "-DCMAKE_CXX_FLAGS=" + " ".join(WINDOWS_FLAGS)]
    log_path = DEPS / f"fltk-{platform}.log"
    print(f"Building static FLTK ({platform}); log: {log_path.relative_to(ROOT)}", flush=True)
    with log_path.open("w") as log:
        run(options, stdout=log, stderr=subprocess.STDOUT)
        run(["cmake", "--build", build, "--parallel", "4"], stdout=log, stderr=subprocess.STDOUT)
        run(["cmake", "--install", build], stdout=log, stderr=subprocess.STDOUT)
    marker.write_text(VERSION)
    return prefix


def assets(platform):
    output = ROOT / "build" / f"studio-{platform}" / "generated"
    output.mkdir(parents=True, exist_ok=True)
    help_text = subprocess.check_output([str(ROOT / "build/vm8"), "help"], text=True)
    values = {
        "help_reference": help_text,
        "demo_source": (ROOT / "programs/demo.asm").read_text(),
        "popcount_source": (ROOT / "programs/popcount.asm").read_text(),
        "system_guide": (ROOT / "docs/HOW_IT_WORKS.md").read_text(),
        "project_license": (ROOT / "LICENSE").read_text(),
        "third_party_notices": "\n\n".join(
            name + "\n" + (DEPS / f"fltk-{VERSION}" / name).read_text(errors="replace")
            for name in ("COPYING", "png/LICENSE", "jpeg/README", "zlib/README")),
    }
    lines = ["// Generated at build time; all content is embedded in the executable.", "#pragma once"]
    for name, value in values.items():
        lines.append(f"inline constexpr const char {name}[] = {json.dumps(value, ensure_ascii=True)};")
    (output / "assets.h").write_text("\n".join(lines) + "\n")
    return output


def compile_studio(platform):
    prefix = dependencies(platform)
    generated = assets(platform)
    output = ROOT / "build" / f"studio-{platform}"
    objects = output / "obj"
    objects.mkdir(parents=True, exist_ok=True)
    cc = "x86_64-w64-mingw32-gcc" if platform == "windows" else "gcc"
    cxx = "x86_64-w64-mingw32-g++" if platform == "windows" else "g++"
    common = ["-O2", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-Iinclude", "-Iassembler"]
    if platform == "windows":
        common += WINDOWS_FLAGS
    compiled = []
    for source in C_SOURCES:
        obj = objects / (source.replace("/", "_") + ".o")
        run([cc, "-std=c17", *common, "-c", source, "-o", obj])
        compiled.append(obj)
    for source in sorted((ROOT / "studio").glob("*.cpp")):
        obj = objects / (source.name + ".o")
        run([cxx, "-std=c++17", *common, "-isystem", prefix / "include",
             "-I", generated, "-Istudio", "-c", source, "-o", obj])
        compiled.append(obj)
    flags = shlex.split(subprocess.check_output(
        ["sh", str(prefix / "bin/fltk-config"), "--use-images", "--ldstaticflags"], text=True))
    executable = output / ("vm8-studio.exe" if platform == "windows" else "vm8-studio")
    link = ["-static", "-mwindows"] if platform == "windows" else ["-static-libgcc", "-static-libstdc++"]
    run([cxx, *compiled, *link, *flags, "-s", "-o", executable])
    print(f"Built {executable.relative_to(ROOT)}", flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=["deps", "build"])
    parser.add_argument("platform", choices=["linux", "windows"])
    args = parser.parse_args()
    if args.action == "deps":
        dependencies(args.platform)
    else:
        compile_studio(args.platform)
