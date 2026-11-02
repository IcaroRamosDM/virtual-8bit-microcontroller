# Maintaining readable source

Source code, tests and build scripts are intended to be read and maintained by
people. Formatting is part of that work; it does not replace a review of names,
control flow, ownership, error handling or tests.

## Conventions

- C and C++ use two spaces, Allman braces and a 100-column target. Use explicit
  blocks for conditionals and loops, one statement per line and descriptive names.
- Python uses four spaces and Black's 100-column layout. Keep imports at the top
  and separate resource setup, work and cleanup visibly.
- Make recipes use tabs. Text files use UTF-8 and LF line endings.
- Prefer short, cohesive functions. Extract a helper when it owns a responsibility
  or removes real duplication; avoid layers that only forward a call.
- Use named constants for limits and explain non-obvious invariants. Avoid
  comments that merely repeat a statement. Keep operations outside test assertions.
- Preserve readable source and generators. Do not reformat dependencies, generated
  output or historical snapshots.

`.editorconfig`, `.clang-format` and `pyproject.toml` record the mechanical rules.
Long literal strings and tables may exceed the target when wrapping would make
their meaning harder to follow.

## Repeatable formatting

The checked versions are **clang-format 18.1.8** and **Black 25.9.0**. Formatting
is optional for building or running the app. With these tools available:

```sh
make format-check
make format
git diff --check
```

The helper checks `build/format-tools/bin/` first, then `PATH`. Explicit tool paths
are also supported, without modifying the machine's global configuration:

```sh
CLANG_FORMAT=/path/to/clang-format BLACK=/path/to/black make format-check
```

The helper only visits maintained desktop source directories. It never downloads
tools, changes Git's index or formats `build/`. Review its resulting diff before
committing. A formatting change should preserve literals and behavior; keep
functional fixes separately identifiable in the review.

## Validation

```sh
make test studio-timing-test studio-file-test
make studio-build-tools-test
make studio-test
make studio-windows windows-test-build studio-file-windows-build
make studio-appimage
make studio-deb
make studio-desktop-test
```

The GUI test uses Xvfb. Windows cross-compilation verifies the build, and Linux
package tests verify local installation; neither replaces testing the application
on the target Windows or Linux desktop. Desktop installation tests use temporary
directories, leaving the user's installed application intact.
