#!/usr/bin/env python3
"""Package an existing Studio AppImage or install its desktop integration."""

import argparse
import hashlib
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
APPIMAGE = ROOT / "build/studio-linux/VM8_Studio-x86_64.AppImage"
PACKAGE_VERSION = "1.1.0-1"
APP_ID = "vm8-studio"
MANAGED_KEY = "X-VM8-Managed=true"


def run(arguments, **kwargs):
    return subprocess.run([str(value) for value in arguments], check=True, **kwargs)


def desktop_string(value):
    return (str(value).replace("\\", "\\\\").replace("\n", "\\n")
            .replace("\r", "\\r").replace("\t", "\\t"))


def desktop_command(path):
    # Exec has a quoting layer after the desktop file's string escaping layer.
    value = str(path)
    if any(character in value for character in ("\n", "\r", "=", "%")):
        raise ValueError("The installation path cannot contain newlines, '=' or '%'.")
    for character in ("\\", '"', "`", "$"):
        value = value.replace(character, "\\" + character)
    return desktop_string('"' + value + '"')


def desktop_entry(executable, icon, managed=False):
    lines = (ROOT / "studio/vm8-studio.desktop").read_text().splitlines()
    replacements = {"Exec": desktop_command(executable), "Icon": desktop_string(icon)}
    lines = [line.split("=", 1)[0] + "=" + replacements[line.split("=", 1)[0]]
             if line.split("=", 1)[0] in replacements else line for line in lines]
    if managed:
        lines.append(MANAGED_KEY)
    return "\n".join(lines) + "\n"


def extract_image(image, temporary):
    # The input is a locally built, trusted VM8 release, not an arbitrary download.
    header = image.open("rb")
    with header:
        magic = header.read(11)
    if magic[:5] != b"\x7fELF\x02" or magic[8:11] != b"AI\x02":
        raise ValueError("Expected the x86-64 VM8 Studio type-2 AppImage.")
    executable = temporary / "studio.AppImage"
    shutil.copyfile(image, executable)
    executable.chmod(0o755)
    run([executable, "--appimage-extract"], cwd=temporary,
        stdout=subprocess.DEVNULL)
    appdir = temporary / "squashfs-root"
    required = ("AppRun", "usr/bin/vm8-studio", "usr/share/fonts.conf",
                "usr/share/licenses/VM8-MIT.txt", "vm8-studio.svg")
    for name in required:
        if not (appdir / name).is_file():
            raise ValueError(f"Incomplete VM8 Studio AppImage: missing {name}")
    return appdir


def validate_desktop(path):
    if shutil.which("desktop-file-validate"):
        run(["desktop-file-validate", path])


def write_file(path, content, mode=0o644):
    path.parent.mkdir(parents=True, exist_ok=True)
    # Keep an already installed menu entry valid until its replacement is complete.
    with tempfile.NamedTemporaryFile(dir=path.parent, delete=False) as stream:
        temporary = Path(stream.name)
        stream.write(content.encode("utf-8"))
    try:
        temporary.chmod(mode)
        temporary.replace(path)
    finally:
        temporary.unlink(missing_ok=True)


def require_managed(path):
    if path.is_symlink() or (path.exists() and MANAGED_KEY not in path.read_text().splitlines()):
        raise ValueError(f"Preserving an existing launcher not created by this installer: {path}")


def build_deb(image, version):
    if not re.fullmatch(r"[0-9][A-Za-z0-9.+~\-]*", version):
        raise ValueError("Invalid Debian package version.")
    if not shutil.which("dpkg-deb"):
        raise ValueError("Building the installer requires dpkg-deb on the build host.")
    output = ROOT / "build/studio-linux"
    output.mkdir(parents=True, exist_ok=True)
    destination = output / f"{APP_ID}_{version}_amd64.deb"
    with tempfile.TemporaryDirectory(prefix="studio-deb-", dir=output) as directory:
        temporary = Path(directory)
        appdir = extract_image(image, temporary)
        package = temporary / "package"
        installed_app = package / "opt/vm8-studio"
        installed_app.parent.mkdir(parents=True)
        shutil.move(str(appdir), installed_app)
        write_file(package / "usr/bin/vm8-studio",
                   '#!/bin/sh\nexec /opt/vm8-studio/AppRun "$@"\n', 0o755)
        entry = package / "usr/share/applications/vm8-studio.desktop"
        write_file(entry, desktop_entry("/usr/bin/vm8-studio", APP_ID))
        validate_desktop(entry)
        icon = package / "usr/share/icons/hicolor/scalable/apps/vm8-studio.svg"
        icon.parent.mkdir(parents=True)
        shutil.copyfile(installed_app / "vm8-studio.svg", icon)
        write_file(package / "usr/share/doc/vm8-studio/copyright",
                   (ROOT / "LICENSE").read_text() + "\nBundled component notices and runtime sources: "
                   "/opt/vm8-studio/usr/share/licenses and runtime-sources.\n")
        installed_kib = (sum(path.stat().st_size for path in package.rglob("*")
                             if path.is_file()) + 1023) // 1024
        write_file(package / "DEBIAN/control", f"""Package: {APP_ID}
Version: {version}
Section: education
Priority: optional
Architecture: amd64
Maintainer: Icaro Ramos Rodrigues dos Santos <icaroelt@gmail.com>
Depends: libc6 (>= 2.36)
Installed-Size: {installed_kib}
Homepage: https://github.com/IcaroRamosDM/virtual-8bit-microcontroller
Description: Offline Assembly editor and virtual 8-bit microcontroller
 Write, assemble, execute and debug VM8 Assembly programs, with built-in
 examples and searchable help. Includes desktop menu integration, GUI
 libraries and fonts. Uses X11 or XWayland; no FUSE mount is required.
""")
        artifact = temporary / destination.name
        run(["dpkg-deb", "--root-owner-group", "--build", package, artifact])
        artifact.replace(destination)
    print(f"Installer: {destination}")
    return destination


def install_user(image, data_home, desktop_shortcut=False):
    data_home = Path(data_home).expanduser()
    if not data_home.is_absolute():
        raise ValueError("The desktop data directory must be an absolute path.")
    digest = hashlib.sha256(image.read_bytes()).hexdigest()
    installation = data_home / APP_ID / digest
    entry = data_home / "applications/vm8-studio.desktop"
    icon = data_home / "icons/hicolor/scalable/apps/vm8-studio.svg"
    content = desktop_entry(installation / "AppRun", icon, managed=True)
    require_managed(entry)
    shortcut = None
    if desktop_shortcut:
        desktop = Path(run(["xdg-user-dir", "DESKTOP"], capture_output=True,
                           text=True).stdout.strip())
        if not desktop.is_absolute() or desktop == Path.home():
            raise ValueError("No separate desktop folder is configured; omit --desktop-shortcut.")
        shortcut = desktop / "VM8 Studio.desktop"
        require_managed(shortcut)
    installation.parent.mkdir(parents=True, exist_ok=True)
    if not installation.exists():
        with tempfile.TemporaryDirectory(prefix=".install-", dir=installation.parent) as directory:
            appdir = extract_image(image, Path(directory))
            # The fingerprint names immutable releases. Older releases remain intact.
            appdir.rename(installation)
    run([installation / "AppRun", "--self-test"])
    icon.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(installation / "vm8-studio.svg", icon)
    write_file(entry, content)
    validate_desktop(entry)
    if shutil.which("update-desktop-database"):
        run(["update-desktop-database", entry.parent])
    if shortcut:
        write_file(shortcut, content, 0o755)
        if shutil.which("gio"):
            result = subprocess.run(["gio", "set", str(shortcut),
                                     "metadata::trusted", "true"], check=False)
            if result.returncode:
                print("Right-click the desktop shortcut and choose Allow Launching once.")
        print(f"Desktop shortcut: {shortcut}")
    print(f"Application: {installation / 'AppRun'}")
    print(f"Menu entry: {entry}")
    print("Search your applications for VM8 Studio to open it.")
    return entry


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("deb", "install-user"))
    parser.add_argument("--appimage", type=Path, default=APPIMAGE)
    parser.add_argument("--version", default=PACKAGE_VERSION)
    parser.add_argument("--data-home", type=Path,
                        default=Path(os.environ.get("XDG_DATA_HOME") or Path.home() / ".local/share"))
    parser.add_argument("--desktop-shortcut", action="store_true")
    options = parser.parse_args()
    try:
        if options.action == "deb":
            build_deb(options.appimage.resolve(), options.version)
        else:
            install_user(options.appimage.resolve(), options.data_home, options.desktop_shortcut)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        parser.exit(1, f"VM8 Studio desktop setup failed: {error}\n")


if __name__ == "__main__":
    main()
