# Virtual 8-bit Microcontroller

A learning project that implements an 8-bit virtual microcontroller in C and executes programs written in a custom Assembly language.

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
- command-line parsing and built-in help for simulator commands and CPU instructions.

Public headers use `#pragma once`. The memory size is derived from the complete 8-bit address space, and the implementation starts from a fully zero-initialized CPU state.
The current cycle counter is intentionally simplified: every attempted instruction counts as one cycle regardless of its byte length.

## Current instruction set

| Mnemonic | Opcode | Behavior |
| --- | --- | --- |
| `NOP` | `0x00` | Completes one cycle without changing CPU state beyond the program counter and cycle count. |
| `HALT` | `0x01` | Stops instruction execution after the current cycle. |
| `LDI A, imm8` | `0x10` | Loads an 8-bit immediate value into register `A`, updates the zero flag, and preserves the carry flag. |
| `LDI B, imm8` | `0x11` | Loads an 8-bit immediate value into register `B`, updates the zero flag, and preserves the carry flag. |
| `ADD A, B` | `0x20` | Adds register `B` to `A`, stores the low eight bits in `A`, and updates the zero and carry flags. |

An invalid opcode halts execution and produces a distinct step result.
The two `LDI` instructions occupy two bytes each: the opcode followed by the immediate value. `ADD A, B` encodes both registers in its one-byte opcode.

## Current execution flow

The demonstration program loads `0xF0` into `A`, loads `0x20` into `B`, adds the registers, and halts. It loads those bytes through `cpu_load_program` and executes them through `cpu_run` with an instruction limit. The current output is:

```text
Virtual 8-bit microcontroller simulator
Execution result: halted
Register A: 0x10
Register B: 0x20
Zero flag: clear
Carry flag: set
Program counter: 6
Cycle count: 4
```

`main.c` is limited to defining the demonstration program, requesting loading and execution, presenting the result, and returning the process exit status. Program copying and the execution loop remain in testable CPU functions.

The CLI module keeps command parsing and help presentation separate from CPU execution. Run `make help` to see simulator commands, instruction encodings, effects, flag behavior, and usage examples.

## Project structure

- `src/`: C implementation files.
- `include/`: public C headers.
- `assembler/`: future assembler implementation.
- `programs/`: programs written in the custom Assembly language.
- `tests/`: automated tests.
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

Run the automated tests:

```bash
make test
```

The test target builds and runs separate CPU and CLI test executables.

Display simulator and instruction help:

```bash
make help
```

Remove generated files:

```bash
make clean
```
