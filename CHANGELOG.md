# Changelog

## 1.1.0 - 2026-09-10

- Add VM8 Studio for Windows x64 and Linux x86-64: an Assembly editor with syntax
  colors, line numbers, examples, file open/save, and raw-byte export.
- Integrate the C assembler and CPU with live registers, memory and bytecode
  views, input/output ports, breakpoints, instruction highlighting, step,
  run/pause/reset, bounded tracing, and an instruction safety limit.
- Embed searchable offline help, the instruction reference, system guide,
  project license, and third-party notices.
- Show configurable virtual-clock time and active real elapsed time separately.
- Provide a Debian 12-based AppImage and an Ubuntu/Debian installer with an
  application-menu entry, plus optional per-user Linux desktop installation.
- Remove empty trailing lines that produced unnecessary scrollbars in the
  register and timing displays.
- Add in-memory assembly diagnostics and source mappings, Windows cross-builds,
  GUI/timing/installation verification, and updated desktop documentation.
- Keep the existing CPU instruction set, CLI workflows, and VM8 bytecode format.

## 1.0.0 - 2026-09-08

- Release the C17 virtual 8-bit CPU, bounded stack and memory-mapped I/O,
  command-line simulator, interactive monitor, and two-pass assembler.
- Include introductory and Popcount firmware examples, automated C/Bash tests,
  bilingual system guides, and the MIT License.
