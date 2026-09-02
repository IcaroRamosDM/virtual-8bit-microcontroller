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
- single-instruction execution with explicit status results.

Public headers use `#pragma once`. The memory size is derived from the complete 8-bit address space, and the implementation starts from a fully zero-initialized CPU state.

## Current instruction set

| Mnemonic | Opcode | Behavior |
| --- | --- | --- |
| `NOP` | `0x00` | Completes one cycle without changing CPU state beyond the program counter and cycle count. |
| `HALT` | `0x01` | Stops instruction execution after the current cycle. |

An invalid opcode halts execution and produces a distinct step result.

## Current execution flow

The demonstration program writes `NOP` followed by `HALT` to memory and repeatedly calls `cpu_step` until execution stops. The current output is:

```text
Virtual 8-bit microcontroller simulator
Execution result: halted
Program counter: 2
Cycle count: 2
```

`main.c` currently shows this integration explicitly. Reusable program loading and execution loops will move into testable CPU functions as the project grows.

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

Remove generated files:

```bash
make clean
```
