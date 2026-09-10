# VM8 Studio: the portable Assembly workbench

VM8 Studio combines an Assembly editor, the existing C assembler, and the existing C CPU simulator in a graphical desktop application. The editor, two examples, instruction reference, system guide, and project license are embedded in the executable. End users do not need Ubuntu, WSL, GCC, Make, Python, or an Internet connection.

## Which file should I copy?

Download the files attached to [GitHub Releases](https://github.com/IcaroRamosDM/virtual-8bit-microcontroller/releases). The table gives the equivalent paths after building from source; downloaded files can live in any suitable folder.

| Computer | Copy this one file | Start it |
| --- | --- | --- |
| Windows x64 | `build/studio-windows/vm8-studio.exe` | Double-click the `.exe`. |
| Ubuntu/Debian x86-64 desktop | `build/studio-linux/vm8-studio_1.1.0-1_amd64.deb` | Open with the graphical software installer, install, then search for **VM8 Studio**. |
| Linux x86-64 desktop | `build/studio-linux/VM8_Studio-x86_64.AppImage` | Allow execution if necessary, then double-click the AppImage. |

Choose the package for the target operating system. The host is 64-bit; the simulated microcontroller is still 8-bit. ARM computers, 32-bit operating systems, macOS, and non-glibc Linux distributions are not targets of these builds.

### Install in the Ubuntu/Debian application menu

The `.deb` installer provides an installed Linux application with its own icon and
searchable **VM8 Studio** menu entry. Open the package in Software Install (or the
distribution's equivalent), click **Install**, and authorize the installation if
asked. Then press the Super/Windows key, search for **VM8 Studio**, and click its
icon. Subsequent launches need no terminal. The software manager can remove the
application later. A graphical `.deb` installer must be available on the desktop;
this package does not install a software manager itself.

The package installs the existing portable application's contents under
`/opt/vm8-studio`, plus a launcher, menu entry and icon in the standard system
directories. It includes the same libraries, fonts and offline help as the
AppImage, retains the glibc 2.36+ and X11/XWayland requirements, and starts without
mounting an AppImage or needing FUSE. No CPU or assembler behavior changes.

For developer use, `make studio-install-user` instead installs a private copy in
the current account, adds the menu entry, and creates a desktop shortcut. It uses
`XDG_DATA_HOME` or `~/.local/share`, stores releases by their AppImage fingerprint,
and does not depend on the repository's `build/` directory afterward. Repeating
the command reuses the identical release. It preserves pre-existing custom
launchers and does not delete previous releases or user source documents.
The desktop shortcut may require **Allow Launching** once if the desktop cannot
set its trusted-launcher metadata automatically. This setup does not require root.

Prefer one installation method per account. If migrating from the developer's
private installation to the system `.deb`, remove its managed
`~/.local/share/applications/vm8-studio.desktop` entry and desktop shortcut so they
do not continue pointing at the older private copy. Saved Assembly documents are
separate from either application installation.

### Run the portable AppImage

The Linux package is built on Debian 12 and targets glibc 2.36 or newer, using X11 or XWayland. It bundles its GUI libraries and fonts. This includes a baseline suitable for Debian 12/13, Ubuntu 24.04 or newer, and other sufficiently recent glibc-based desktop distributions, but it is not a claim of testing every distribution or desktop environment. Ubuntu 22.04 has an older glibc and is not covered by this build.

On Linux, a download or a Windows-formatted USB drive may lose the executable permission. Use your file manager's file properties and enable execution if that option is available. The equivalent exact command, from the file's folder, is:

```bash
chmod +x VM8_Studio-x86_64.AppImage
```

Then double-click the file. The desktop must permit running executables and provide FUSE for the usual AppImage launch. The selected runtime includes libfuse; there is no separate libfuse2 installation requirement. Environments without usable FUSE can run the same file with its extraction fallback:

```bash
APPIMAGE_EXTRACT_AND_RUN=1 ./VM8_Studio-x86_64.AppImage
```

The Windows build uses only Windows system DLLs. It is an unsigned application, so Windows or an organization's application-control policy may display a warning or block it. Do not disable security controls: on a managed computer, ask its administrator to approve the application. Code signing and public release-asset upload are separate distribution steps, not performed by a local build.

## First run: the included Popcount firmware

1. Open the application. The Popcount source is already in the editor. To reload it later, choose **Examples > Popcount**.
2. Leave **Input (0xEE)** at `0xA5`, or enter that value and click **Apply**.
3. Click **Assemble**. The event log should report `44 / 238 bytes`, `26 instructions`, and `12 symbols`. These are static counts in the source, not the number of instructions a loop will execute.
4. Click **Run**, or press **F5**. The program counts the four set bits in `0xA5`.
5. Check the right-hand CPU panel: `OUT = 0x04`, `SP = 0x00`, `PC = 0x1C`, and `Cycles: 126`, with state `HALTED`.

The GUI displays PC in hexadecimal: `0x1C` is decimal `28`, as printed by the CLI. The output also appears in binary as `00000100`. Select **Live memory** to inspect the ports at addresses `0xEE` and `0xEF`.

For another run, change Input to `0xFF`, click **Reset**, then **Run**. Expect `OUT = 0x08` and `138` cycles. With input `0x00`, expect `OUT = 0x00` and `114` cycles. Reset applies the current Input field and reloads the assembled program.

## Write your own program

Choose **File > New**, replace the editor contents with the following program, and choose **File > Save As** to save it as `addition.asm` in a writable folder:

```asm
.EQU OUTPUT_PORT, 0xEF

start:
  LDI A, 0x2A
  LDI B, 0x01
  ADD A, B
  STA OUTPUT_PORT
  HALT
```

Click **Assemble**, then **Run**. Expected final state:

- `A = 0x2B`, `B = 0x01`, and `OUT = 0x2B` (decimal 43).
- `PC = 0x08`, `SP = 0x00`, zero and carry flags clear.
- `Cycles: 5`, state `HALTED`, and eight assembled bytes.

The **Assembled bytes** tab shows the real VM8 program:

```text
10 2A 11 01 20 41 EF 01
```

Each row includes an address, byte, source line, and instruction mnemonic or operand/data indication. The assembled-byte count is the graphical counterpart of `wc -c`, and this byte view is the counterpart of `od -An -tx1 -v`. The original shell commands remain useful when developing through the CLI; see the bilingual system guides for those examples.

`File > Export binary` optionally saves these raw bytes to a `.bin` file. That file is VM8 bytecode, not a Windows or Linux executable. Studio does not need to create a `.bin` file to run the source in its editor.

## Observe execution one instruction at a time

With the addition program assembled, click **Reset**, then **Step** twice. Expect `A = 0x2A`, `B = 0x01`, `PC = 0x04`, and two cycles. One more Step executes `ADD`: A becomes `0x2B`, PC becomes `0x05`, and the cycle count becomes three.

To use a breakpoint:

1. Reset the CPU.
2. Put the text cursor on the line containing `ADD A, B`.
3. Press **F9** or click **Breakpoint**. The event log reports address `0x04`.
4. Press **F5**. Execution stops **before** ADD, with two cycles completed.
5. Press **F10** to execute ADD once, or **F5** to continue running.

The next instruction is highlighted in teal. Breakpoint and assembly-error lines use a reddish background; the current-PC highlight takes precedence if both refer to the same line. Reset preserves breakpoints. Assembling changed source clears old breakpoints because instruction addresses may have changed. Comments, labels, and `.BYTE` data lines are not independent instruction breakpoints.

## Virtual clock and real execution time

The CPU panel shows two different measurements. Neither requires an external tool:

- **Virtual time** is the accumulated instruction cycle count divided by the active virtual clock frequency. VM8 counts one cycle per attempted instruction, including HALT and failed opcode/stack operations. Instruction byte length does not change that count. This is a simplified timing model, not physical hardware timing.
- **Real elapsed** is a monotonic stopwatch on the host computer. It accumulates active Run intervals, stops on Pause/HALT/error/breakpoint/safety limit/source edit, and resumes with Run. Step adds its CPU-step and trace-formatting time, excluding the user's delay between clicks and the subsequent screen refresh. During Run, it includes Speed scheduling delays and the GUI/trace work performed while running: it is **not a CPU-only benchmark**.

### Calculate virtual execution time

1. Choose **Examples > Popcount**, leave Input at `0xA5`, and click **Assemble**, then **Run**.
2. The initial **Clock (Hz)** is `1000000` (1 MHz). Expect `126` cycles and **Virtual time: 126.000 us**, because `126 / 1000000 = 0.000126` seconds.
3. Enter `1000` in **Clock (Hz)** and click the **Apply** button on that same row, or press Enter inside the clock field. **Active: 1000 Hz** confirms the applied frequency. The existing 126 cycles now show **Virtual time: 126.000 ms**. Registers, memory, cycle count, and Real elapsed do not reset or otherwise change.
4. Apply `2000000` (2 MHz). The same execution now represents **63.000 us**. Restore `1000000` to return to the default model.

The field accepts decimal integers from `1` to `1000000000` Hz, without units, spaces, decimal points, signs, or thousands separators. Invalid input leaves the active frequency unchanged. Editing the field alone does not apply it: Run and Reset use the **Active** value. A new clock recalculates **all** accumulated cycles at the selected frequency; Studio does not integrate a history of different clock frequencies or pace execution to that frequency.

### Compare the host stopwatch

Run Popcount with `0xA5` at 1 MHz using **Speed > Observe**. Note both measurements. Click **Reset**, select **Fast**, and Run again. Virtual time stays at **126.000 us**; Real elapsed will normally be shorter with Fast. Exact real values depend on the computer, system load, rendering and tracing, so no fixed expected stopwatch result is prescribed. Turning Trace off reduces overhead, but the measurement still includes Run's visualization pacing.

Pause while the program is running, wait a few seconds, then continue with Run. The stopwatch stays frozen during that pause; waiting time is not added when execution resumes. The displayed value refreshes with the GUI and is frozen after execution stops.

**Reset**, a successful **Assemble**, **New**, **Open**, and loading an example clear both times. The active clock stays selected for the current Studio session; restarting the application restores 1 MHz. Failed assembly retains the paused CPU/time snapshot while preventing stale bytecode from running. Applying Input or clearing the log does not reset time. Units adjust automatically: `ns` means nanoseconds, `us` microseconds, `ms` milliseconds, and `s` seconds. Displayed decimals are rounded and do not guarantee matching measurement accuracy.

## Controls and shortcuts

| Control | Shortcut | Meaning |
| --- | --- | --- |
| New / Open / Save | Ctrl+N / Ctrl+O / Ctrl+S | Create, open, or save Assembly text. |
| Save As | Ctrl+Shift+S | Choose another source-file path. |
| Find | Ctrl+F | Search the source; search wraps to its beginning. |
| Assemble | Ctrl+Enter | Translate the editor text, then reset and load the CPU on success. |
| Run | F5 | Assemble changed source if necessary, then execute from the current PC. |
| Pause | F6 | Stop automatic execution without resetting the CPU. |
| Step | F10 | Execute one instruction, ignoring a breakpoint at the current PC. |
| Reset | Ctrl+R | Clear CPU state and reload the assembled program. |
| Clock (Hz) / Apply | Enter in the clock field | Apply the virtual frequency and recalculate virtual time. |
| Breakpoint | F9 | Toggle a breakpoint on the instruction at the editor cursor. |
| Help | F1 | Open the searchable offline reference. |

**Speed** selects one, 16, or 128 instructions per approximately 20 ms interface update. This is a visualization rate, not a model of real hardware clock frequency. The CPU's existing cycle model still counts one cycle per attempted instruction.

**Trace** enables per-instruction rows in the event log. The log retains a bounded amount of text, discarding old rows when necessary. **Clear** clears only the log. Live memory includes ordinary RAM, decoded I/O, and the stack; the bytecode tab shows the originally assembled image, not later self-modifications to live memory.

## Help, errors, and safeguards

Press **F1** and select an instruction on the left. Each instruction has its syntax, opcode, byte length, compact effect notation, plain-language explanation, flag behavior, and examples from the CLI's maintained reference. Search also finds terms inside topics, such as `carry`, `borrow`, `stack`, `.EQU`, and `memory`. The full CLI reference and the detailed English system guide are embedded too.

For example, `LDI A, 0x2A` puts a literal value into A; `LDA 0x2A` reads a value from memory address `0x2A`. Search for LDI and LDA to compare their reference entries.

An error such as `LDI A 1` is reported with its source line and missing-comma explanation. The editor highlights that line. A failed assembly does not silently execute a previous binary. Editing source pauses execution and invalidates the assembled mapping until the source is assembled again.

One Run is limited to 10,000 attempted instructions. A loop such as the following pauses at that limit without freezing the window:

```asm
loop:
  JMP loop
```

You can inspect the state, change the program, reset, or Run again. This limit is independent of the 238-byte program capacity. The editor accepts source files up to 64 KiB, with at most 255 bytes per physical Assembly line; comments and symbol spelling consume host memory, not simulated program bytes.

Closing the window, loading an example, or replacing edited source prompts to Save, Discard, or Cancel. Programs are saved only when requested. The application's directory can be read-only; source files may be saved elsewhere.

## Rebuild and test (developers only)

The core remains C17; `studio/` is a separate C++17/FLTK 1.4.5 presentation layer. `assembler/assembly.c` adds an in-memory entry point using the existing first/second passes. Per-operation diagnostic callbacks send errors to the GUI without replacing global stderr; the CLI keeps its original stderr behavior. Source-to-byte mappings and breakpoints live on the host, not inside the VM8's 256-byte address space.

On an Ubuntu 24.04 x86-64 build host, install the desktop build and test tools:

```bash
sudo apt update
sudo apt install --no-install-recommends \
  build-essential cmake python3 pkg-config ca-certificates curl \
  gcc-mingw-w64-x86-64-win32 g++-mingw-w64-x86-64-win32 \
  libx11-dev libxext-dev libxft-dev libxinerama-dev libxcursor-dev \
  squashfs-tools xvfb xauth dpkg-dev desktop-file-utils xdg-user-dirs
```

The portable build requires **PRoot 5.4.0 or newer**. Ubuntu 24.04's PRoot 5.1.0
can make Debian's APT incorrectly report missing signing keys because it does
not handle the newer `faccessat2` system call. Use an updated PRoot instead of
changing signature verification or importing unrelated keys. The following
installs the tested official x86-64 PRoot 5.4.1 binary after verifying its checksum:

```bash
(
  set -eu
  proot_download=$(mktemp -d)
  trap 'rm -rf "$proot_download"' EXIT
  curl --fail --location --output "$proot_download/proot" \
    https://github.com/proot-me/proot/releases/download/v5.4.1/proot
  printf '%s  %s\n' \
    19f44283f5c0e73091c60195f5fcd4f4c1165505e44410d434e2ab1b677c1a09 \
    "$proot_download/proot" | sha256sum --check
  sudo install -m 0755 "$proot_download/proot" /usr/local/bin/proot
)
proot --version
```

Python 3.11 or newer is required. From the repository root, build and verify in
this order; desktop packaging reuses the AppImage produced in the preceding step:

```bash
make test
make studio-test
make studio-windows
make studio-appimage
make studio-deb
make studio-desktop-test
```

FLTK is downloaded from its official release and checked against a pinned SHA-256 digest. The AppImage target creates a separate Debian 12 filesystem under `build/studio-portable/`, installs build packages inside that filesystem, and compiles there so that the binary does not inherit the newer Ubuntu host's glibc requirement. The base image and AppImage runtime are checksum-pinned. First-time builds need Internet access and additional disk space; the resulting application runs offline.

Keep the repository and portable-build cache on a Linux filesystem for faster builds. Set `VM8_PORTABLE_CACHE` to a dedicated build directory to override the default cache location. PRoot's optional seccomp acceleration is disabled for compatibility with kernels where that acceleration is unreliable; no host security policy is changed. This build environment is a compatibility tool, not a security sandbox for untrusted code.

`make studio-linux` builds an unpackaged developer executable against the current host. Use `make studio-appimage` for the distributable Linux artifact. `make studio-test` runs backend checks, deterministic timing tests, and application actions on a virtual display, saving screenshots under `build/studio-smoke/`. The GUI checks include actual event-loop execution, clock changes and rejection, pause/resume, reset, and timer stops on breakpoints, instruction limits and CPU errors. The `--self-test` and `--ui-smoke-test <existing-output-directory>` arguments are developer test hooks, not required for ordinary use.

After producing the AppImage, `make studio-deb` creates the Ubuntu/Debian installer
with `dpkg-deb`; it reuses the finished release and does not download dependencies
or rebuild the CPU/GUI. `make studio-desktop-test` exercises installation into a
temporary account-data directory, repeated installation, preservation of custom
launchers, path validation and invalid package input. Desktop entry metadata
follows the [freedesktop specification](https://specifications.freedesktop.org/desktop-entry/latest/recognized-keys.html).

`make studio-timing-test` runs just the C++ timing tests without FLTK or a display. The tests inject monotonic timestamps, so pause/resume checks do not depend on sleep durations or machine speed. `make studio-timing-windows-build` cross-compiles the same tests; run `.\build\windows-tests\test_studio_timing.exe` in PowerShell from the repository root to verify them natively on Windows. These are separate from the existing C/CLI test suite.

The `build/studio-smoke*` directories contain test output and screenshots. Older `studio-timing-final-*` directories are previous validation snapshots, not release-selection rules. Distribute the newly built files listed at the top of this guide. If moving the repository, rebuild the FLTK installation caches at the new location: CMake and `fltk-config` can retain absolute build paths. Preserve the old cache as a backup until the new build passes.

Generated executables, dependency caches, screenshots, and AppImages remain inside ignored build directories. See the [release checklist](RELEASING.md) for testing the packaged files and uploading them. Building does not stage, commit, push, sign, create a GitHub release, or upload release assets.
