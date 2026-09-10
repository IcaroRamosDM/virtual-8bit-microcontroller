#!/usr/bin/env python3
"""Build a portable x86-64 AppImage on an isolated, pinned Debian 12 base."""

import argparse
import hashlib
import os
from pathlib import Path
import re
import shutil
import subprocess
import tarfile
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
CACHE = Path(os.environ.get("VM8_PORTABLE_CACHE", ROOT / "build/studio-portable")).resolve()
BASE_COMMIT = "bae6d64d90b4068b09ff9d8b564c2773ef5d8d83"
BASE_HASH = "abf56b2f87242de589f03ea56779358079c07c4c099bd1e454d083538eb6666d"
BASE_URL = f"https://raw.githubusercontent.com/debuerreotype/docker-debian-artifacts/{BASE_COMMIT}/bookworm/oci/blobs/rootfs.tar.gz"
RUNTIME_URL = "https://github.com/AppImage/type2-runtime/releases/download/continuous/runtime-x86_64"
RUNTIME_HASH = "1cc49bcf1e2ccd593c379adb17c9f85a36d619088296504de95b1d06215aebbf"
SYSROOT = CACHE / "debian12"
PACKAGES = ["build-essential", "cmake", "python3", "ca-certificates", "pkg-config",
            "libx11-dev", "libxext-dev", "libxft-dev", "libxinerama-dev", "libxcursor-dev",
            "fonts-dejavu-core"]


def download(url, path, digest):
    if not path.exists():
        print(f"Downloading {path.name} from the official upstream", flush=True)
        urllib.request.urlretrieve(url, path)
    if hashlib.sha256(path.read_bytes()).hexdigest() != digest:
        raise RuntimeError(f"Checksum mismatch: {path}. Nothing was extracted or executed.")


def isolated(arguments, **kwargs):
    command = ["proot", "-0", "-r", str(SYSROOT), "-b", "/dev", "-b", "/proc", "-b", "/sys",
               "-b", "/etc/resolv.conf:/etc/resolv.conf", "-w", "/work", "/usr/bin/env",
               "DEBIAN_FRONTEND=noninteractive", "LC_ALL=C", *arguments]
    # PRoot's optional seccomp acceleration crashes on some recent WSL kernels.
    # Disabling that optimization retains PRoot's normal ptrace-based isolation.
    environment = dict(os.environ, PROOT_NO_SECCOMP="1")
    return subprocess.run(command, check=True, env=environment, **kwargs)


def require_proot():
    executable = shutil.which("proot")
    if not executable:
        raise RuntimeError("PRoot 5.4.0+ is required; see docs/STUDIO.md for setup.")
    output = subprocess.check_output([executable, "--version"], text=True,
                                     stderr=subprocess.STDOUT)
    version = re.search(r"\bv(\d+)\.(\d+)\.(\d+)", output)
    if not version or tuple(map(int, version.groups())) < (5, 4, 0):
        raise RuntimeError(
            "PRoot 5.4.0+ is required for Debian filesystem access checks. "
            "Older versions can cause false APT signing-key errors; "
            "see docs/STUDIO.md for the verified PRoot 5.4.1 installation.")


def prepare():
    require_proot()
    CACHE.mkdir(parents=True, exist_ok=True)
    archive = CACHE / "debian12-rootfs.tar.gz"
    download(BASE_URL, archive, BASE_HASH)
    marker = SYSROOT / ".vm8-base-ready"
    if not marker.exists():
        SYSROOT.mkdir(parents=True, exist_ok=True)
        with tarfile.open(archive) as package:
            for member in package:
                if member.name.startswith("/") or ".." in Path(member.name).parts:
                    raise RuntimeError("Unsafe rootfs archive member")
        # The verified OS image legitimately contains absolute symlinks. GNU tar
        # extracts those links without following them into the host filesystem.
        subprocess.run(["tar", "-xzf", str(archive), "-C", str(SYSROOT), "--no-same-owner"], check=True)
        (SYSROOT / "work").mkdir(exist_ok=True)
        log_path = CACHE / "debian-setup.log"
        print(f"Preparing isolated Debian build tools; log: {log_path}", flush=True)
        with log_path.open("w") as log:
            isolated(["apt-get", "update"], stdout=log, stderr=subprocess.STDOUT)
            isolated(["apt-get", "install", "-y", "--no-install-recommends", *PACKAGES], stdout=log, stderr=subprocess.STDOUT)
        marker.write_text(BASE_COMMIT + "\n")


def build():
    prepare()
    work = SYSROOT / "work"
    for directory in ("src", "include", "assembler", "studio", "tools", "programs", "docs"):
        shutil.copytree(ROOT / directory, work / directory, dirs_exist_ok=True)
    for filename in ("Makefile", "LICENSE"):
        shutil.copy2(ROOT / filename, work / filename)
    # Reuse only the checksum-verified source archive, never Ubuntu-built objects.
    fltk_archive = ROOT / "build/studio-deps/fltk-1.4.5-source.tar.gz"
    if fltk_archive.exists():
        destination = work / "build/studio-deps"
        destination.mkdir(parents=True, exist_ok=True)
        shutil.copy2(fltk_archive, destination / fltk_archive.name)
    log_path = CACHE / "debian-build.log"
    print(f"Building Studio on Debian 12 (glibc 2.36); log: {log_path}", flush=True)
    with log_path.open("w") as log:
        isolated(["make", "studio-linux"], stdout=log, stderr=subprocess.STDOUT)
    isolated(["/work/build/studio-linux/vm8-studio", "--self-test"])


def package():
    executable = SYSROOT / "work/build/studio-linux/vm8-studio"
    if not executable.exists():
        raise RuntimeError("Build the Debian executable first")
    output = ROOT / "build/studio-linux"
    output.mkdir(parents=True, exist_ok=True)
    # A fresh AppDir prevents obsolete libraries from surviving a rebuild.
    import tempfile
    appdir = Path(tempfile.mkdtemp(prefix="VM8-Studio-", suffix=".AppDir", dir=CACHE))
    (appdir / "usr/bin").mkdir(parents=True)
    (appdir / "usr/lib").mkdir()
    shutil.copy2(executable, appdir / "usr/bin/vm8-studio")
    dependencies = isolated(["ldd", "/work/build/studio-linux/vm8-studio"], capture_output=True, text=True).stdout
    system_libraries = {"libc.so.6", "libm.so.6", "libpthread.so.0", "libdl.so.2", "librt.so.1"}
    bundled = []
    for name, path in re.findall(r"\s+(\S+) => (/\S+)", dependencies):
        if name in system_libraries:
            continue
        shutil.copy2(SYSROOT / path.lstrip("/"), appdir / "usr/lib" / name)
        bundled.append(name)
    fonts = appdir / "usr/share/fonts"
    shutil.copytree(SYSROOT / "usr/share/fonts/truetype/dejavu", fonts)
    shutil.copy2(ROOT / "studio/AppRun", appdir / "AppRun")
    (appdir / "AppRun").chmod(0o755)
    shutil.copy2(ROOT / "studio/fonts.conf", appdir / "usr/share/fonts.conf")
    shutil.copy2(ROOT / "studio/vm8-studio.desktop", appdir / "vm8-studio.desktop")
    shutil.copy2(ROOT / "studio/vm8-studio.svg", appdir / "vm8-studio.svg")
    (appdir / ".DirIcon").symlink_to("vm8-studio.svg")
    notices = appdir / "usr/share/licenses"
    notices.mkdir()
    shutil.copy2(ROOT / "LICENSE", notices / "VM8-MIT.txt")
    shutil.copy2(SYSROOT / "work/build/studio-deps/fltk-1.4.5/COPYING", notices / "FLTK.txt")
    # Include the Debian copyright notices and referenced common license texts.
    # This also covers static GCC runtime libraries and the embedded font files.
    for copyright_file in (SYSROOT / "usr/share/doc").glob("*/copyright"):
        shutil.copy2(copyright_file, notices / (copyright_file.parent.name.replace(":", "-") + ".txt"))
    shutil.copytree(SYSROOT / "usr/share/common-licenses", notices / "common-licenses", symlinks=False)
    # The runtime statically links LGPL libfuse. Ship its corresponding source,
    # the upstream patch, and the runtime source/build scripts inside the AppImage.
    sources = appdir / "usr/share/runtime-sources"
    sources.mkdir()
    runtime_sources = [
        ("type2-runtime-source.tar.gz",
         "https://codeload.github.com/AppImage/type2-runtime/tar.gz/75849dce7cc37e4319b633df1f116ca895c71a12",
         "b7af4960da4b90364e935a3281d04fad6560da4813c012414fa2f738291ad443"),
        ("fuse-3.15.0.tar.xz",
         "https://github.com/libfuse/libfuse/releases/download/fuse-3.15.0/fuse-3.15.0.tar.xz",
         "70589cfd5e1cff7ccd6ac91c86c01be340b227285c5e200baa284e401eea2ca0"),
        ("squashfuse-0.5.2.tar.gz",
         "https://github.com/vasi/squashfuse/archive/0.5.2.tar.gz",
         "db0238c5981dabbd80ee09ae15387f390091668ca060a7bc38047912491443d3"),
    ]
    for name, url, digest in runtime_sources:
        archive = CACHE / name
        download(url, archive, digest)
        shutil.copy2(archive, sources / name)
    with tarfile.open(CACHE / "type2-runtime-source.tar.gz") as source_archive:
        license_entry = next(member for member in source_archive if member.name.endswith("/LICENSE"))
        (notices / "AppImage-runtime.txt").write_bytes(source_archive.extractfile(license_entry).read())
    (notices / "BUILD-ORIGINS.txt").write_text(
        f"VM8 Studio uses unmodified FLTK 1.4.5 with its static-linking exception.\n"
        f"FLTK source: https://github.com/fltk/fltk/releases/tag/release-1.4.5\n"
        f"Debian 12 base: {BASE_URL}\nSHA256: {BASE_HASH}\n"
        "Debian library sources: https://sources.debian.org/\n"
        "AppImage runtime source: https://github.com/AppImage/type2-runtime/tree/75849dce7cc37e4319b633df1f116ca895c71a12\n"
        "The runtime and patched libfuse corresponding sources are in ../runtime-sources/.\n"
        "FreeType is distributed under the FreeType License (FTL) option.\n"
        "This software is based in part on the work of the FreeType Team.\n"
        "Bundled libraries:\n" + "\n".join(bundled) + "\n")
    runtime = CACHE / "appimage-runtime-x86_64"
    download(RUNTIME_URL, runtime, RUNTIME_HASH)
    squashfs = appdir.with_suffix(".squashfs")
    subprocess.run(["mksquashfs", str(appdir), str(squashfs), "-noappend", "-comp", "gzip",
                    "-all-root", "-no-progress", "-processors", "2"], check=True, stdout=subprocess.DEVNULL)
    image = output / "VM8_Studio-x86_64.AppImage"
    with image.open("wb") as destination:
        for source in (runtime, squashfs):
            with source.open("rb") as stream:
                shutil.copyfileobj(stream, destination)
    image.chmod(0o755)
    # Keep the matching unpackaged executable for developer diagnostics.
    shutil.copy2(executable, output / "vm8-studio")
    print(f"Packaged {image} ({image.stat().st_size:,} bytes)", flush=True)
    print(f"AppDir retained for inspection: {appdir}", flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("prepare", "build", "package", "all"), default="all", nargs="?")
    arguments = parser.parse_args()
    if arguments.action == "prepare": prepare()
    if arguments.action in ("build", "all"): build()
    if arguments.action in ("package", "all"): package()
