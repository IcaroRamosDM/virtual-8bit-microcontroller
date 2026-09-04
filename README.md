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
- an 8-bit stack pointer;
- 256 bytes of unified memory, partitioned into a 240-byte program/data region and a 16-byte stack;
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
- explicit stack-overflow and stack-underflow execution results;
- a bounded binary-file reader that rejects input larger than the 240-byte program capacity;
- a standalone two-pass assembler with normalized source reading, a shared label-and-constant symbol table, strict instruction and directive parsing, byte-operand resolution, instruction encoding, and raw binary output;
- command-line selection between normal or traced execution of the built-in demonstration and an external binary program;
- built-in help for simulator commands and CPU instructions;
- process-level CLI tests for successful execution and expected failure paths.

Public headers use `#pragma once`. The memory size is derived from the complete 8-bit address space, and the implementation starts from a fully zero-initialized CPU state.
The stack occupies addresses `0xF0` through `0xFF`, grows downward, and uses `SP = 0x00` as its empty sentinel rather than as a stack-memory address.
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
| `AND A, B` | `0x22` | Stores the bitwise AND of `A` and `B` in `A`, updates zero, and clears carry. |
| `OR A, B` | `0x23` | Stores the bitwise OR of `A` and `B` in `A`, updates zero, and clears carry. |
| `XOR A, B` | `0x24` | Stores the bitwise exclusive OR of `A` and `B` in `A`, updates zero, and clears carry. |
| `NOT A` | `0x25` | Inverts every bit in `A`, updates zero, and clears carry. |
| `SHL A` | `0x26` | Shifts `A` left with zero fill, updates zero, and moves the original bit 7 into carry. |
| `SHR A` | `0x27` | Shifts `A` right with zero fill, updates zero, and moves the original bit 0 into carry. |
| `CMP A, B` | `0x28` | Compares unsigned `A` with `B`, updates zero and borrow, and leaves both registers unchanged. |
| `JZ addr8` | `0x30` | Jumps to an absolute 8-bit address when the zero flag is set; otherwise execution continues after its operand. |
| `JNZ addr8` | `0x32` | Jumps to an absolute 8-bit address when the zero flag is clear. |
| `JC addr8` | `0x33` | Jumps to an absolute 8-bit address when the carry flag is set. |
| `JMP addr8` | `0x31` | Jumps unconditionally to an absolute 8-bit address without changing registers or flags. |
| `LDA addr8` | `0x40` | Loads register `A` from an absolute memory address, updates the zero flag, and preserves the carry flag. |
| `STA addr8` | `0x41` | Stores register `A` at an absolute memory address without changing registers or flags. |
| `PUSH A` | `0x50` | Pushes `A` onto the downward-growing stack without changing registers or flags. |
| `POP A` | `0x51` | Pops the newest stack byte into `A`, updates zero, and preserves carry. |

An invalid opcode, stack overflow, or stack underflow halts execution and produces a distinct step result.
The two `LDI` instructions, `JZ`, `JNZ`, `JC`, `JMP`, `LDA`, and `STA` occupy two bytes each: the opcode followed by an immediate value, target address, or data address. Arithmetic, comparison, logical, and shift instructions occupy one byte because their required register operands are implied by the opcode.
`PUSH A` and `POP A` are also one-byte instructions because register `A` and the stack operation are completely identified by their opcodes.

The carry flag reports unsigned carry for addition, unsigned borrow for subtraction and comparison, and the bit shifted out by `SHL` or `SHR`. The four non-shift logical instructions clear carry. `CMP` sets zero when `A == B` and carry when unsigned `A < B`, without changing either register. `JZ`, `JNZ`, and `JC` test the corresponding flag while preserving CPU state other than the program counter and cycle count. `JMP` always replaces the program counter with its absolute address operand. `LDA` reads memory into `A` and updates zero, while `STA` writes `A` to memory without changing flags.

## Assembler symbols and directives

Labels and named constants are both symbols and share one case-sensitive namespace. A label records the current output address, while `.EQU` associates a name with a literal 8-bit value. Neither one emits a byte by itself.

`.BYTE` emits exactly one raw byte. Its operand may be a decimal or `0x` hexadecimal literal, a named constant, or a label:

```asm
.EQU DATA_ADDRESS, 0x80
.EQU INITIAL_VALUE, 0x5A

start:
  LDA initial_data
  STA DATA_ADDRESS
  HALT

initial_data:
  .BYTE INITIAL_VALUE
```

The assembler resolves `initial_data` to its byte address, `DATA_ADDRESS` to `0x80`, and `INITIAL_VALUE` to `0x5A`. Only the resulting bytes enter VM8 memory; the symbol names remain host-side assembly text.

## Current execution flow

The built-in demonstration loads `0x2A` into both registers, subtracts `B` from `A`, and uses the resulting zero flag to branch over an `LDI A, 0xFF` instruction. It then loads `0x5A`, stores it at memory address `0x80`, clears `A`, reloads the stored value, and halts.

The Assembly demonstration produces the same final CPU state but is intentionally not byte-for-byte identical. It names repeated values with `.EQU`, places `0x5A` after `HALT` with `.BYTE`, and obtains that value with `LDA initial_data`. Its generated binary therefore contains 19 bytes, while the built-in program contains 18. `HALT` remains at address `0x11`, so the final program counter is still `0x12` (decimal 18); the byte at `0x12` is data and is not executed.

Both programs are loaded through `cpu_load_program` and executed through `cpu_run_with_observer` with an instruction limit. Normal execution passes no observer; trace execution passes the trace callback and `stdout`. The current normal output is:

```text
Virtual 8-bit microcontroller simulator
Execution result: halted
Register A: 0x5A
Register B: 0x2A
Stack pointer: 0x00
Zero flag: clear
Carry flag: clear
Program counter: 18
Cycle count: 9
```

The demonstration bytecode is stored privately in `src/program.c` and exposed through a `Program` value containing a pointer to constant bytes and their size. Running `make run` selects this built-in program.

The Assembly demonstration is stored in `programs/demo.asm`. The standalone assembler resolves its labels, constants, instructions, and data directives in two passes and generates the behaviorally equivalent 19-byte raw machine-code file at `build/demo.bin`. Running `make run-bin` assembles that source, reads the generated binary into a bounded 240-byte host buffer, copies the resulting program into CPU memory, and executes it.

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
  ADDR=0x00 OP=0x10 MNEMONIC=LDI A=0x2A B=0x00 SP=0x00 Z=0 C=0 NEXT=0x02 CYCLES=1 RESULT=ok
```

`ADDR` is the address at which the instruction started, `OP` is its raw opcode byte, `MNEMONIC` is the decoded operation name, `SP` is the post-execution stack pointer, and `NEXT` is the post-execution program counter. The remaining fields show registers `A` and `B`, the zero and carry flags, the accumulated cycle count, and the step result. An unrecognized byte is displayed as `MNEMONIC=UNKNOWN`.

Run the automated tests:

```bash
make test
```

The test target assembles `programs/demo.asm` and then builds and runs independent tests for the CPU, CPU observer, instruction-set lookup, trace formatter, CLI, built-in program, binary reader, assembled-program execution, source normalization and reading, symbol table, first pass, byte parsing and resolution, instruction parser and encoder, second pass, and binary writer.
The CPU tests cover arithmetic, logical operations, unary bit inversion, shifted-out carry bits, nondestructive comparison, every taken and non-taken conditional branch, stack ordering and boundaries, stack error propagation, zero results, and a `JMP`-to-zero loop that verifies bounded execution stops at the configured instruction limit.
The program-integration test loads and executes the built-in demonstration, then verifies its complete final CPU state and the value stored at data address `0x80`.
The assembled-program integration test reads `build/demo.bin`, loads it into CPU memory, executes it, verifies the same final CPU state, and checks both the embedded byte at `0x12` and the copied value at `0x80`. This confirms that the human-readable Assembly source and built-in byte array describe behaviorally equivalent programs even though their byte sequences differ.
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

This runs both assembler passes and writes 19 raw bytes to `build/demo.bin`: 18 bytes through the `HALT` instruction followed by one embedded data byte.

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
19 build/demo.bin
```

Inspect every byte in hexadecimal:

```bash
od -An -tx1 -v build/demo.bin
```

Expected result:

```text
 10 2a 11 2a 21 30 09 10 ff 40 12 41 80 10 00 40
 80 01 5a
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
