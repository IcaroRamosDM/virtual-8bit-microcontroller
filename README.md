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
- an immutable bytecode-program descriptor that keeps its byte pointer and size together;
- a dedicated module for the built-in demonstration bytecode;
- a standalone assembler with normalized source reading, a symbol table, a first pass, and 8-bit literal and symbol resolution;
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
| `SUB A, B` | `0x21` | Subtracts register `B` from `A`, stores the wrapped 8-bit result in `A`, and sets carry when an unsigned borrow occurs. |
| `JZ addr8` | `0x30` | Jumps to an absolute 8-bit address when the zero flag is set; otherwise execution continues after its operand. |
| `JMP addr8` | `0x31` | Jumps unconditionally to an absolute 8-bit address without changing registers or flags. |
| `LDA addr8` | `0x40` | Loads register `A` from an absolute memory address, updates the zero flag, and preserves the carry flag. |
| `STA addr8` | `0x41` | Stores register `A` at an absolute memory address without changing registers or flags. |

An invalid opcode halts execution and produces a distinct step result.
The two `LDI` instructions, `JZ`, `JMP`, `LDA`, and `STA` occupy two bytes each: the opcode followed by an immediate value, target address, or data address. `ADD A, B` and `SUB A, B` encode both registers in their one-byte opcodes.

The carry flag reports unsigned carry for addition and unsigned borrow for subtraction. `JZ` always fetches its address operand; a taken branch replaces the program counter with that address, while a non-taken branch continues at the following byte. `JMP` always replaces the program counter with its absolute address operand. `LDA` reads memory into `A` and updates zero, while `STA` writes `A` to memory without changing flags.

## Current execution flow

The demonstration program loads `0x2A` into both registers, subtracts `B` from `A`, and uses the resulting zero flag to branch over an `LDI A, 0xFF` instruction. It then loads `0x5A`, stores it at memory address `0x80`, clears `A`, reloads the stored value, and halts. The program is loaded through `cpu_load_program` and executed through `cpu_run` with an instruction limit. The current output is:

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

The demonstration bytecode is stored privately in `src/program.c` and exposed through a `Program` value containing a pointer to constant bytes and their size. `main.c` is limited to requesting that program, coordinating loading and execution, presenting the result, and returning the process exit status. Program copying and the execution loop remain in testable CPU functions.

The CLI module keeps command parsing and help presentation separate from CPU execution. Run `make help` to see simulator commands, instruction encodings, effects, flag behavior, and usage examples.

## Project structure

- `src/`: CPU, CLI, built-in program, and simulator-entry-point implementations.
- `include/`: public C headers.
- `assembler/`: standalone assembler implementation.
- `programs/`: programs written in the custom Assembly language, including the current demonstration source.
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

The test target builds and runs separate CPU, CLI, and program-integration test executables.
The CPU tests include a `JMP`-to-zero loop that verifies bounded execution stops at the configured instruction limit.
The program-integration test loads and executes the built-in demonstration, then verifies its complete final CPU state and the value stored at data address `0x80`.

Display simulator and instruction help:

```bash
make help
```

Build the current assembler:

```bash
make assembler
```

Build the assembler when necessary and analyze the demonstration source:

```bash
make assemble
```

At the current milestone, `vm8asm` normalizes source statements, builds a symbol table, calculates instruction addresses and program size in its first pass, parses 8-bit literals, and resolves literal or symbolic byte operands. Second-pass instruction encoding and binary-output generation are not implemented yet, so this command does not create `build/demo.bin`.

Remove generated files:

```bash
make clean
```
