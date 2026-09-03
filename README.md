# Virtual 8-bit Microcontroller

A learning project that implements an 8-bit virtual microcontroller in C and executes programs written in a custom Assembly language.

## Detailed documentation

- [How the system works (English)](docs/HOW_IT_WORKS.md)
- [How the system works (Brazilian Portuguese)](docs/HOW_IT_WORKS.pt-BR.md)

## Development rules

- All repository content must be written in English.
- Source code is written and edited with Vim.
- Documentation and other prose are written and edited with Nano.
- The project is built and tested inside Ubuntu on WSL.
- Each cohesive feature or productive development session is recorded with a focused Git commit.

## Current architecture

The initial CPU model contains:

- two 8-bit general-purpose registers named `A` and `B`;
- an 8-bit program counter;
- 256 bytes of unified code and data memory;
- zero and carry flags;
- a halted state;
- a 64-bit cycle counter;
- byte-level memory read and write operations;
- byte fetching with automatic program-counter advancement;
- single-instruction execution with explicit status results;
- validated program loading into unified memory;
- bounded program execution with explicit termination results;
- an optional per-instruction observer that receives post-execution CPU snapshots;
- a human-readable execution trace for built-in and file-loaded programs;
- shared instruction-set metadata that maps raw opcode bytes to decoded mnemonics;
- an immutable bytecode-program descriptor that keeps its byte pointer and size together;
- a dedicated module for the built-in demonstration bytecode;
- a bounded binary-file reader that rejects input larger than the 256-byte memory capacity;
- a standalone two-pass assembler with normalized source reading, a symbol table, strict instruction parsing, byte-operand resolution, instruction encoding, and raw binary output;
- command-line selection between normal or traced execution of the built-in demonstration and an external binary program;
- built-in help for simulator commands and CPU instructions;
- process-level CLI tests for successful execution and expected failure paths.

Public headers use `#pragma once`. The memory size is derived from the complete 8-bit address space, and the implementation starts from a fully zero-initialized CPU state.
The current cycle counter is intentionally simplified: every attempted instruction counts as one cycle regardless of its byte length.
The `instruction_set` module owns the opcode definitions and their mnemonic lookup, allowing the CPU, assembler, help, built-in program, and trace to share the same opcode vocabulary without making opcode-only modules depend on the complete CPU interface.

## Current instruction set

| Mnemonic | Opcode | Behavior |
| --- | --- | --- |
| `NOP` | `0x00` | Completes one cycle without changing CPU state beyond the program counter and cycle count. |
| `HALT` | `0x01` | Stops instruction execution after the current cycle. |
| `LDI A, imm8` | `0x10` | Loads an 8-bit immediate value into register `A`, updates the zero flag, and preserves the carry flag. |
| `LDI B, imm8` | `0x11` | Loads an 8-bit immediate value into register `B`, updates the zero flag, and preserves the carry flag. |
| `ADD A, B` | `0x20` | Adds register `B` to `A`, stores the low eight bits in `A`, and updates the zero and carry flags. |
| `SUB A, B` | `0x21` | Subtracts register `B` from `A`, stores the wrapped 8-bit result in `A`, and sets carry when an unsigned borrow occurs. |
| `JZ addr8` | `0x30` | Jumps to an absolute 8-bit address when the zero flag is set; otherwise execution continues after its operand. |
| `JMP addr8` | `0x31` | Jumps unconditionally to an absolute 8-bit address without changing registers or flags. |
| `LDA addr8` | `0x40` | Loads register `A` from an absolute memory address, updates the zero flag, and preserves the carry flag. |
| `STA addr8` | `0x41` | Stores register `A` at an absolute memory address without changing registers or flags. |

An invalid opcode halts execution and produces a distinct step result.
The two `LDI` instructions, `JZ`, `JMP`, `LDA`, and `STA` occupy two bytes each: the opcode followed by an immediate value, target address, or data address. `ADD A, B` and `SUB A, B` encode both registers in their one-byte opcodes.

The carry flag reports unsigned carry for addition and unsigned borrow for subtraction. `JZ` always fetches its address operand; a taken branch replaces the program counter with that address, while a non-taken branch continues at the following byte. `JMP` always replaces the program counter with its absolute address operand. `LDA` reads memory into `A` and updates zero, while `STA` writes `A` to memory without changing flags.

## Current execution flow

The demonstration program loads `0x2A` into both registers, subtracts `B` from `A`, and uses the resulting zero flag to branch over an `LDI A, 0xFF` instruction. It then loads `0x5A`, stores it at memory address `0x80`, clears `A`, reloads the stored value, and halts. The program is loaded through `cpu_load_program` and executed through `cpu_run_with_observer` with an instruction limit. Normal execution passes no observer; trace execution passes the trace callback and `stdout`. The current normal output is:

```text
Virtual 8-bit microcontroller simulator
Execution result: halted
Register A: 0x5A
Register B: 0x2A
Zero flag: clear
Carry flag: clear
Program counter: 18
Cycle count: 9
```

The demonstration bytecode is stored privately in `src/program.c` and exposed through a `Program` value containing a pointer to constant bytes and their size. Running `make run` selects this built-in program.

The same demonstration is also written in `programs/demo.asm`. The standalone assembler resolves its labels in two passes and generates the equivalent 18-byte raw machine-code file at `build/demo.bin`. Running `make run-bin` assembles that source, reads the generated binary into a bounded 256-byte host buffer, copies the resulting program into CPU memory, and executes it.

An arbitrary compatible binary can be selected with `./build/vm8 run <program.bin>`. The CLI distinguishes the built-in and external-binary execution modes and independently enables tracing when requested. `main.c` remains responsible for orchestration, presentation, and process status. Reusable file reading, program copying, CPU execution, and trace formatting remain in independently tested modules. Run `make help` to see simulator commands, instruction encodings, effects, flag behavior, and usage examples.

## Project structure

- `src/`: CPU, trace formatter, CLI, binary reader, built-in program, and simulator-entry-point implementations.
- `include/`: public C headers.
- `assembler/`: standalone assembler implementation.
- `programs/`: programs written in the custom Assembly language, including the current demonstration source.
- `tests/`: automated C unit and integration tests plus Bash process-level tests.
- `build/`: ignored generated files.
- `Makefile`: build automation.

## Build

Compile the project:

```bash
make
```

Compile and run:

```bash
make run
```

Assemble `programs/demo.asm`, load the generated binary, and run it:

```bash
make run-bin
```

Run another compatible binary directly:

```bash
./build/vm8 run path/to/program.bin
```

Trace the built-in demonstration instruction by instruction:

```bash
make trace
```

Assemble and trace `programs/demo.asm`:

```bash
make trace-bin
```

Trace another compatible binary directly:

```bash
./build/vm8 trace path/to/program.bin
```

Each trace entry identifies the executed instruction address and opcode, then shows the CPU state after that instruction:

```text
Execution trace:
  ADDR=0x00 OP=0x10 MNEMONIC=LDI A=0x2A B=0x00 Z=0 C=0 NEXT=0x02 CYCLES=1 RESULT=ok
```

`ADDR` is the address at which the instruction started, `OP` is its raw opcode byte, `MNEMONIC` is the decoded operation name, and `NEXT` is the post-execution program counter. The remaining fields show registers `A` and `B`, the zero and carry flags, the accumulated cycle count, and the step result. An unrecognized byte is displayed as `MNEMONIC=UNKNOWN`.

Run the automated tests:

```bash
make test
```

The test target assembles `programs/demo.asm` and then builds and runs independent tests for the CPU, CPU observer, instruction-set lookup, trace formatter, CLI, built-in program, binary reader, assembled-program execution, source normalization and reading, symbol table, first pass, byte parsing and resolution, instruction parser and encoder, second pass, and binary writer.
The CPU tests include a `JMP`-to-zero loop that verifies bounded execution stops at the configured instruction limit.
The program-integration test loads and executes the built-in demonstration, then verifies its complete final CPU state and the value stored at data address `0x80`.
The assembled-program integration test reads `build/demo.bin`, loads it into CPU memory, executes it, and verifies the same final state. This confirms that the human-readable Assembly source and built-in byte array describe equivalent programs.
The Bash process test launches `build/vm8` exactly as a user would and verifies normal external-binary execution, both trace modes, and the expected failure behavior for a missing file, an empty file, an oversized file, and an invalid opcode.

Display simulator and instruction help:

```bash
make help
```

Build the current assembler:

```bash
make assembler
```

Build the assembler when necessary and generate the demonstration binary:

```bash
make assemble
```

This runs both assembler passes and writes 18 raw bytes to `build/demo.bin`.

The equivalent direct command is:

```bash
./build/vm8asm programs/demo.asm build/demo.bin
```

Inspect the generated file size:

```bash
wc -c build/demo.bin
```

Expected result:

```text
18 build/demo.bin
```

Inspect every byte in hexadecimal:

```bash
od -An -tx1 -v build/demo.bin
```

Expected result:

```text
 10 2a 11 2a 21 30 09 10 ff 10 5a 41 80 10 00 40
 80 01
```

Here, `-An` suppresses the address column, `-tx1` selects hexadecimal one-byte units, and `-v` prevents repeated data from being abbreviated. These commands inspect the binary without interpreting it as text.

Assemble and run both inspections with one target:

```bash
make inspect
```

Remove generated files:

```bash
make clean
```
