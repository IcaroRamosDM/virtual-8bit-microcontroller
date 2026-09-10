# Running the Linux and Windows command-line executables

For a complete double-click application with an Assembly editor and offline Help, download **VM8 Studio** from [GitHub Releases](https://github.com/IcaroRamosDM/virtual-8bit-microcontroller/releases): the `.exe` for Windows, the `.deb` installer for Ubuntu/Debian, or the portable Linux AppImage. See [the Studio guide](STUDIO.md). The remainder of this document describes the separate terminal tools, not the graphical application.

VM8 includes two host programs: the microcontroller simulator (`vm8`) and its two-pass assembler (`vm8asm`). The simulator's interactive interface is a terminal monitor, not a graphical window. The simulated CPU uses 8-bit data regardless of the host's 64-bit architecture.

## Choose the correct folder

| Folder | Simulator | Assembler |
| --- | --- | --- |
| `build/linux/` | `vm8` | `vm8asm` |
| `build/windows/` | `vm8.exe` | `vm8asm.exe` |

Each folder also contains `demo.asm`, `demo.bin`, `popcount.asm`, `popcount.bin`, `LICENSE`, and this guide as `USAGE.md`. Copy the complete platform folder when you want the examples and instructions with the executables. The simulator itself includes a built-in demonstration; external programs require their `.bin` files.

The `.bin` files are VM8 bytecode, not Windows or Linux applications. The same bytecode works with either simulator. Do not double-click a `.bin` file or rename a Linux executable to `.exe`.

## Windows: PowerShell

From the repository root in PowerShell, enter:

```powershell
Set-Location .\build\windows
```

If you copied the platform folder somewhere else, use its actual path instead. All commands below run from the folder containing `vm8.exe` and `vm8asm.exe`.

Show simulator commands and the complete instruction reference:

```powershell
.\vm8.exe help
```

Run the built-in demonstration or the Assembly demonstration:

```powershell
.\vm8.exe run
.\vm8.exe run .\demo.bin
```

Both finish with `Register A: 0x5A`, `Register B: 0x2A`, `Program counter: 18`, and `Cycle count: 9`.

Run and trace the popcount firmware:

```powershell
.\vm8.exe run .\popcount.bin --input 0xA5
.\vm8.exe trace .\popcount.bin --input 0xA5
```

Open the interactive monitor:

```powershell
.\vm8.exe monitor .\popcount.bin
```

At the `vm8>` prompt, enter:

```text
input 0xA5
run
registers
quit
```

Expect `Output port: 0x04`, `Stack pointer: 0x00`, `Program counter: 28`, and `Cycle count: 126`. Use `help` at that prompt to see stepping, memory inspection, tracing, and breakpoint commands.

Assemble a source file and execute its output without Ubuntu or WSL:

```powershell
.\vm8asm.exe .\demo.asm .\demo-custom.bin
.\vm8.exe run .\demo-custom.bin
```

Check the process result immediately after a command with `$LASTEXITCODE`; successful normal execution returns `0`. Double-clicking `vm8.exe` runs the short built-in demonstration and may close the console immediately. Start it from an existing PowerShell window, with `monitor` when you want an interactive session.

## Linux: terminal

From the repository root on Linux:

```bash
cd build/linux
```

If you copied the Linux folder elsewhere, enter that folder instead. If the transfer did not preserve executable permissions, restore them:

```bash
chmod +x vm8 vm8asm
```

The equivalent Linux commands are:

```bash
./vm8 help
./vm8 run
./vm8 run ./demo.bin
./vm8 run ./popcount.bin --input 0xA5
./vm8 trace ./popcount.bin --input 0xA5
./vm8 monitor ./popcount.bin
```

Monitor commands and expected CPU results are identical to the Windows examples above. `quit` leaves the monitor.

Assemble and execute a source file locally:

```bash
./vm8asm ./demo.asm ./demo-custom.bin
./vm8 run ./demo-custom.bin
```

Check `echo $?` immediately after the command whose result you want to inspect. Successful normal execution returns `0`.

## Expected program sizes and results

| Program | Binary size | Selected input | Important result |
| --- | ---: | --- | --- |
| `demo.bin` | 19 bytes | Default `0x00` | `A = 0x5A`, `B = 0x2A`, `PC = 18`, 9 cycles. |
| `popcount.bin` | 44 bytes | `0x00` | Output `0x00`, 114 cycles. |
| `popcount.bin` | 44 bytes | `0xA5` | Output `0x04`, 126 cycles. |
| `popcount.bin` | 44 bytes | `0xFF` | Output `0x08`, 138 cycles. |

Inspect the same bytes on Windows:

```powershell
(Get-Item .\demo.bin).Length
Format-Hex .\demo.bin
```

Or on Linux:

```bash
wc -c demo.bin
od -An -tx1 -v demo.bin
```

## Rebuilding from source

Build commands run from the repository root inside Ubuntu/Linux, not inside the platform output folders. GCC with C17 support and GNU Make build the Linux programs. The Windows cross-compiler builds native Windows x64 executables from the same C sources.

On Ubuntu, install the cross-compiler once:

```bash
sudo apt update
sudo apt install --no-install-recommends gcc-mingw-w64-x86-64-win32
```

Build one platform or both:

```bash
make linux
make windows
make release
```

`make windows` uses a Linux-host assembler to prepare the example bytecode; it does not require Wine or execute Windows programs during the build. The Windows assembler can subsequently assemble new programs directly on Windows. Native builds use optimization with `-O2`; the existing development targets retain debug information and remain unchanged.

Windows compilation sets `__USE_MINGW_ANSI_STDIO=1` to use MinGW's standard C formatting support, including `%zu` for `size_t`. The `-fno-builtin-printf`, `-fno-builtin-fprintf`, and `-fno-builtin-snprintf` flags prevent GCC's legacy Windows built-in format assumptions from conflicting with that choice. The MinGW headers still supply GNU-format checking: all strict warnings and `-Werror` remain enabled, and portable format strings in the C sources are unchanged. Static compiler-runtime linking avoids requiring separate MinGW DLLs alongside the executables; Windows system DLLs are still used.

The existing verification and usage commands still work:

```bash
make test
make run
make help
make inspect
make inspect-popcount
```

`make clean` removes generated files under `build/`, including both platform folders. `make release` regenerates them; it does not create a Git tag, upload binaries, or change an existing GitHub release.

## Native Windows C tests

The normal `make test` runs the 23 C test executables and the 15 Bash process cases on Linux. To compile the same C tests for Windows, run this from the repository root on Ubuntu/Linux:

```bash
make windows-test-build
```

This target also prepares `build/demo.bin` and `build/popcount.bin`, which the integration tests need. It compiles the tests but does not execute them. Copy the resulting `build/windows-tests/` directory and the two demonstration `.bin` files under `build/` to the same relative locations in a Windows checkout. Then, in PowerShell, run from that repository root (not `build/windows/`):

```powershell
Get-ChildItem .\build\windows-tests\test_*.exe | ForEach-Object {
    & $_.FullName
    if ($LASTEXITCODE -ne 0) { throw "Test failed: $($_.Name)" }
}
```

Expect all 23 test executables to pass. Some intentionally print invalid-input diagnostics before reporting success. The Bash process suite remains a Linux test; the direct Windows commands in this guide exercise the native simulator and assembler.

The state, trace, and monitor tests use `tests/test_stream.h` for temporary streams. On Linux it calls `tmpfile()`. On Windows it creates exclusive binary-mode files under `build/`, with automatic deletion when closed. This avoids the drive-root location documented for [Microsoft's legacy `tmpfile`](https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/tmpfile) and does not modify the simulator or assembler.

If Windows reports that an Application Control policy blocked a generated executable, that process has not run and its test result is unavailable. Do not disable Windows security or request administrator access just to bypass the block; check the applicable policy with the device administrator. A successful Linux test run does not replace native Windows validation.

## Compatibility and working on different computers

- The prepared Windows executables target Windows x64 and use Windows system libraries. They do not require Ubuntu, WSL, GCC, or a separate MinGW runtime installation to run.
- The prepared Linux executables target x86-64 and require glibc 2.34 or newer, as verified from their imported symbol versions. They are dynamically linked, so this is not a universal build for every distribution or architecture. Inspect dependencies with `ldd ./vm8`. Rebuild on the target Linux system when its architecture or glibc is incompatible. Alpine/musl and ARM systems need a suitable native build.
- Linux and Windows output folders must not be interchanged. The `.bin` examples, however, are intentionally platform-independent.
- Use Git to synchronize source changes between computers. Build outputs are local to each checkout and are not synchronized by Git.
- Rebuild from the same source revision on each build host, or transfer the selected finished platform files. Preserve destination work before replacing files.
- Keep generated executables out of source commits. Distribute desktop executables as release assets; see the [release checklist](RELEASING.md).
