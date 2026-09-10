# Desktop release checklist

VM8 Studio uses GitHub Releases for Windows and Linux downloads. Source commits
contain the application, build scripts, tests, and documentation. Generated
binaries and dependency caches stay under the ignored `build/` directory.

## Prepare a version

Update `PACKAGE_VERSION` in `tools/linux_desktop.py`, the installer filename in
`README.md` and `docs/STUDIO.md`, and `CHANGELOG.md`. The current desktop release
is `v1.1.0`, with Debian package version `1.1.0-1`. Preserve existing published tags.

Install the build prerequisites in [the Studio guide](STUDIO.md). From the
repository root, run:

```bash
make test
make studio-test
make studio-windows
make studio-appimage
make studio-deb
make studio-desktop-test
make help
git diff --check
```

The portable target builds against Debian 12 even when the host uses a newer
Ubuntu version. Do not substitute the host-built `vm8-studio` for the AppImage.
`make studio-deb` packages the already-built AppImage, so always rebuild the
AppImage before creating a new installer.

## Check the files that will be distributed

Test the finished AppImage directly, including its extraction fallback and a
smaller display:

```bash
APPIMAGE_EXTRACT_AND_RUN=1 ./build/studio-linux/VM8_Studio-x86_64.AppImage --self-test
mkdir -p build/studio-release-check/default build/studio-release-check/small
APPIMAGE_EXTRACT_AND_RUN=1 xvfb-run -a -s '-screen 0 1600x1000x24' \
  ./build/studio-linux/VM8_Studio-x86_64.AppImage \
  --ui-smoke-test build/studio-release-check/default
APPIMAGE_EXTRACT_AND_RUN=1 xvfb-run -a -s '-screen 0 1024x768x24' \
  ./build/studio-linux/VM8_Studio-x86_64.AppImage \
  --ui-smoke-test build/studio-release-check/small
```

Inspect the screenshots and confirm that registers and both timing lines are
readable. Check installer metadata with
`dpkg-deb --info build/studio-linux/vm8-studio_1.1.0-1_amd64.deb`.
A desktop installation should open through the **VM8 Studio** application-menu
entry. For a private installation on the build host, use `make studio-install-user`.

Cross-compilation is not a native Windows execution test. On Windows x64,
open `vm8-studio.exe`, assemble and run Popcount with input `0xA5`, and expect
output `0x04` and 126 cycles. Also exercise file open/save, breakpoints, step,
pause, reset, timing, and F1 Help. The native timing tests can be built with
`make studio-timing-windows-build` and run on Windows as described in the
[Studio guide](STUDIO.md). Record any platform checks that could not be performed.

## Collect release assets

Keep a dedicated directory containing only the finished files for the selected
version. For `v1.1.0`:

```bash
mkdir -p build/releases/v1.1.0
cp build/studio-windows/vm8-studio.exe build/releases/v1.1.0/
cp build/studio-linux/VM8_Studio-x86_64.AppImage build/releases/v1.1.0/
cp build/studio-linux/vm8-studio_1.1.0-1_amd64.deb build/releases/v1.1.0/
(
  cd build/releases/v1.1.0
  sha256sum vm8-studio.exe VM8_Studio-x86_64.AppImage \
    vm8-studio_1.1.0-1_amd64.deb > SHA256SUMS
  sha256sum --check SHA256SUMS
)
```

Write `build/releases/v1.1.0/RELEASE_NOTES.md` from the changelog, with download
choices, compatibility requirements, and actual verification results. Attach
only the three application files and `SHA256SUMS`; the notes become the release
body. Keep bundled component notices and the AppImage runtime's corresponding
source archives inside the Linux packages. End users need those complete
packages, not the FLTK build tree or the test executables.

## Publish the reviewed revision

Review `git status`, `git diff`, and the names of every new file. Stage only the
reviewed source, test, build-script, and documentation paths. Then review
`git diff --cached --check` and `git diff --cached` before committing with a
focused English Conventional Commit message. Confirm that the working tree
contains no remaining release changes.

Create an annotated `v1.1.0` tag on the verified commit, push that commit and tag,
and create a GitHub Release from the existing tag. Upload the exact files from
`build/releases/v1.1.0/`. The GitHub web interface supports these steps, or an
authenticated GitHub CLI can upload the assets:

```bash
gh release create v1.1.0 --verify-tag --latest \
  --title 'VM8 Studio v1.1.0' \
  --notes-file build/releases/v1.1.0/RELEASE_NOTES.md \
  build/releases/v1.1.0/vm8-studio.exe \
  build/releases/v1.1.0/VM8_Studio-x86_64.AppImage \
  build/releases/v1.1.0/vm8-studio_1.1.0-1_amd64.deb \
  build/releases/v1.1.0/SHA256SUMS
```

After publishing, check the tag's commit, the asset names and sizes, and the
checksums of downloaded files. A source push alone does not upload release
assets. No local build command publishes, signs, or replaces a release.
