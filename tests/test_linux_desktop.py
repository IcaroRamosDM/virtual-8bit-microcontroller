"""Exercise installation from the real, already built Linux release."""

import importlib.util
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("linux_desktop", ROOT / "tools/linux_desktop.py")
desktop = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(desktop)


class DesktopInstallationTests(unittest.TestCase):
    def test_real_installation_and_repeat_preserve_one_release(self):
        if not desktop.APPIMAGE.is_file():
            self.skipTest("Build the Studio AppImage before testing its installation.")
        with tempfile.TemporaryDirectory(prefix="vm8-install-test-") as directory:
            data = Path(directory) / "data with spaces"
            entry = desktop.install_user(desktop.APPIMAGE, data)
            releases = list((data / desktop.APP_ID).iterdir())
            self.assertEqual(len(releases), 1)
            launcher = releases[0] / "AppRun"
            original_inode = launcher.stat().st_ino
            self.assertTrue(launcher.is_file())
            self.assertTrue((data / "icons/hicolor/scalable/apps/vm8-studio.svg").is_file())
            self.assertIn("Name=VM8 Studio\n", entry.read_text())
            self.assertIn('Exec="', entry.read_text())
            self.assertNotIn(str(ROOT / "build"), entry.read_text())
            repeated_entry = desktop.install_user(desktop.APPIMAGE, data)
            self.assertEqual(repeated_entry, entry)
            self.assertEqual(launcher.stat().st_ino, original_inode)
            self.assertEqual(list((data / desktop.APP_ID).iterdir()), releases)

    def test_existing_unmanaged_launcher_is_preserved(self):
        if not desktop.APPIMAGE.is_file():
            self.skipTest("Build the Studio AppImage before testing its installation.")
        with tempfile.TemporaryDirectory(prefix="vm8-preserve-test-") as directory:
            data = Path(directory)
            entry = data / "applications/vm8-studio.desktop"
            entry.parent.mkdir()
            original = "[Desktop Entry]\nName=Existing custom launcher\n"
            entry.write_text(original)
            with self.assertRaisesRegex(ValueError, "Preserving an existing launcher"):
                desktop.install_user(desktop.APPIMAGE, data)
            self.assertEqual(entry.read_text(), original)
            self.assertFalse((data / desktop.APP_ID).exists())

    def test_exec_paths_reject_desktop_field_codes(self):
        with self.assertRaisesRegex(ValueError, "installation path"):
            desktop.desktop_entry("/tmp/profile%f/AppRun", "/tmp/icon.svg")

    def test_reject_invalid_appimage_before_executing_it(self):
        with tempfile.TemporaryDirectory(prefix="vm8-invalid-test-") as directory:
            temporary = Path(directory)
            image = temporary / "invalid.AppImage"
            image.write_text("This is not an executable VM8 release.\n")
            with self.assertRaisesRegex(ValueError, "AppImage"):
                desktop.extract_image(image, temporary)
            self.assertFalse((temporary / "studio.AppImage").exists())

    def test_reject_invalid_package_version(self):
        with self.assertRaisesRegex(ValueError, "package version"):
            desktop.build_deb(desktop.APPIMAGE, "1.0\nDepends: unrelated-package")


if __name__ == "__main__":
    unittest.main()
