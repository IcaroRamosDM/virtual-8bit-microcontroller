# Virtual 8-bit Microcontroller

A learning project that implements an 8-bit virtual microcontroller in C and executes programs written in a custom Assembly language.

## Development rules

- All repository content must be written in English.
- Source code is written and edited with Vim.
- Documentation and other prose are written and edited with Nano.
- The project is built and tested inside Ubuntu on WSL.
- Each meaningful development step is recorded with Git.

## Current architecture

The initial CPU model contains:

- two 8-bit general-purpose registers named `A` and `B`;
- an 8-bit program counter;
- 256 bytes of unified code and data memory;
- zero and carry flags;
- a halted state;
- a 64-bit cycle counter.

Public headers use `#pragma once`. The current implementation starts from a fully zero-initialized CPU state.

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
