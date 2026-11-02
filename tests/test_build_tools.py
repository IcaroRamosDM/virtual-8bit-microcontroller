"""Check build failure recovery without network access or a Debian toolchain."""

import hashlib
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock

TOOLS = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS))
try:
    import downloads
    import package_studio as package
finally:
    sys.path.pop(0)


class DownloadTests(unittest.TestCase):
    def test_interrupted_download_can_be_retried(self):
        with tempfile.TemporaryDirectory() as directory:
            archive = Path(directory) / "source.tar.gz"
            contents = b"complete archive"
            digest = hashlib.sha256(contents).hexdigest()

            def interrupted(url, destination):
                destination.write_bytes(b"partial")
                raise OSError("connection interrupted")

            with mock.patch.object(downloads.urllib.request, "urlretrieve", interrupted):
                with self.assertRaisesRegex(OSError, "connection interrupted"):
                    downloads.download_verified("unused", archive, digest)
            self.assertEqual(list(archive.parent.iterdir()), [])

            def complete(url, destination):
                destination.write_bytes(contents)

            with mock.patch.object(downloads.urllib.request, "urlretrieve", complete):
                downloads.download_verified("unused", archive, digest)
            self.assertEqual(archive.read_bytes(), contents)
            with mock.patch.object(downloads.urllib.request, "urlretrieve") as download:
                downloads.download_verified("unused", archive, digest)
            download.assert_not_called()

    def test_corrupt_download_never_becomes_a_cached_input(self):
        with tempfile.TemporaryDirectory() as directory:
            archive = Path(directory) / "source.tar.gz"
            digest = hashlib.sha256(b"expected").hexdigest()

            def corrupt(url, destination):
                destination.write_bytes(b"corrupt")

            with mock.patch.object(downloads.urllib.request, "urlretrieve", corrupt):
                with self.assertRaisesRegex(RuntimeError, "Checksum mismatch"):
                    downloads.download_verified("unused", archive, digest)
            self.assertEqual(list(archive.parent.iterdir()), [])

    def test_corrupt_existing_cache_is_reported_and_preserved(self):
        with tempfile.TemporaryDirectory() as directory:
            archive = Path(directory) / "source.tar.gz"
            archive.write_bytes(b"corrupt")
            with mock.patch.object(downloads.urllib.request, "urlretrieve") as download:
                with self.assertRaisesRegex(RuntimeError, "Checksum mismatch"):
                    downloads.download_verified(
                        "unused", archive, hashlib.sha256(b"expected").hexdigest()
                    )
            download.assert_not_called()
            self.assertEqual(archive.read_bytes(), b"corrupt")


class PackageFailureTests(unittest.TestCase):
    def test_failed_copy_preserves_previous_appimage(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            image = root / "studio.AppImage"
            image.write_bytes(b"previous release")
            runtime = root / "appimage-runtime-x86_64"
            runtime.write_bytes(b"runtime")
            squashfs = root / "studio.squashfs"
            squashfs.write_bytes(b"filesystem")
            original_files = set(root.iterdir())

            def fail_after_partial_write(source, destination):
                destination.write(b"partial")
                raise OSError("disk full")

            with (
                mock.patch.object(package, "CACHE", root),
                mock.patch.object(package, "download_verified"),
                mock.patch.object(package.subprocess, "run"),
                mock.patch.object(package.shutil, "copyfileobj", fail_after_partial_write),
            ):
                with self.assertRaisesRegex(OSError, "disk full"):
                    package.write_appimage(root / "AppDir", squashfs, image)
            self.assertEqual(image.read_bytes(), b"previous release")
            self.assertEqual(set(root.iterdir()), original_files)

    def test_failed_packaging_removes_staging_directory(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            sysroot = root / "debian12"
            executable = sysroot / "work/build/studio-linux/vm8-studio"
            executable.parent.mkdir(parents=True)
            executable.write_bytes(b"executable")
            cache = root / "cache"

            def fail_after_staging(appdir, source):
                appdir.mkdir()
                (appdir / "partial").write_bytes(b"partial")
                raise OSError("staging failed")

            with (
                mock.patch.object(package, "ROOT", root),
                mock.patch.object(package, "SYSROOT", sysroot),
                mock.patch.object(package, "CACHE", cache),
                mock.patch.object(package, "populate_appdir", fail_after_staging),
            ):
                with self.assertRaisesRegex(OSError, "staging failed"):
                    package.package()
            self.assertEqual(list(cache.iterdir()), [])
            self.assertEqual(executable.read_bytes(), b"executable")


if __name__ == "__main__":
    unittest.main()
