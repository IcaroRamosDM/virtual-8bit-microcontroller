# How the Virtual 8-bit Microcontroller Works

[Leia esta documentação em português do Brasil.](HOW_IT_WORKS.pt-BR.md)

## Purpose

This project models a small 8-bit microcontroller in software. It has two related parts:

1. The **simulator** implements the CPU, memory, instruction decoder, flags, and execution loop.
2. The **assembler**, named `vm8asm`, translates human-readable Assembly source into the byte sequence understood by the simulated CPU.

The central idea is:

```text
Assembly source written by a person
                |
                v
       vm8asm assembler
                |
                v
Machine code containing only bytes
                |
                v
 Virtual 8-bit CPU simulator
```

The CPU never reads words such as `LDI`, `start`, or `memory_demo`. Those words exist only in the source file and inside the assembler. The final program presented to the CPU contains only numeric bytes.

## Current implementation boundary

The CPU simulator is operational and currently runs an 18-byte demonstration program stored in `src/program.c`.

The assembler currently implements:

- command-line argument validation;
- bounded source-file reading;
- comment and surrounding-whitespace removal;
- delivery of normalized statements through a callback;
- a symbol table;
- a first pass that records labels and calculates program size;
- decimal and hexadecimal 8-bit literal parsing;
- resolution of an operand as either a literal or a symbol.

The second-pass instruction encoder and binary-file writer are still under development. Therefore, `make assemble` currently analyzes `programs/demo.asm`, but does not yet create `build/demo.bin`.

## What “8-bit” means

In this project, 8-bit describes the natural data width of the CPU:

- register `A` stores one 8-bit value;
- register `B` stores one 8-bit value;
- each memory location stores one 8-bit value;
- the program counter stores one 8-bit address;
- arithmetic results retained by the CPU are 8 bits wide.

An 8-bit value has 256 possible bit patterns:

```text
Binary:  00000000 through 11111111
Hex:     0x00 through 0xFF
Decimal: 0 through 255
```

The program counter is also 8 bits wide, so it can address 256 memory locations. Consequently, this version has exactly 256 bytes of memory, at addresses `0x00` through `0xFF`.

Eight-bit does **not** mean that every instruction must occupy one byte. An instruction may be composed of multiple consecutive 8-bit values. The CPU fetches those values one at a time.

Real 8-bit processors also commonly have instructions and addresses wider than 8 bits. “8-bit CPU” normally refers mainly to the register and arithmetic-logic-unit width, not to a universal limit on instruction length.

## CPU state

The `Cpu` structure contains the complete visible state of the virtual processor:

| Component | Width or type | Purpose |
| --- | --- | --- |
| Register `A` | 8 bits | Primary accumulator used by arithmetic and memory-transfer instructions. |
| Register `B` | 8 bits | Secondary arithmetic operand. |
| Program counter (`PC`) | 8 bits | Address of the next byte to fetch. |
| Zero flag (`Z`) | Boolean | Indicates that the most recent flag-updating result was zero. |
| Carry flag (`C`) | Boolean | Indicates addition carry-out or subtraction borrow. |
| Halted state | Boolean | Prevents further instruction execution after a halt or invalid opcode. |
| Cycle counter | 64 bits | Counts attempted instructions in the simplified timing model. |
| Memory | 256 bytes | Stores both program bytes and data bytes. |

The normal initial state is completely zero-initialized:

```c
Cpu cpu = {0};
```

The program is then copied into memory beginning at address `0x00`.

## Unified code and data memory

The project uses a unified memory model. Instructions and ordinary data occupy the same 256-byte array.

For example, the demonstration program occupies addresses `0x00` through `0x11`, while it uses address `0x80` to store data. The instruction:

```asm
STA 0x80
```

writes register `A` into memory location `0x80`.

Because code and data share the same array, a store directed at a program address could overwrite an instruction. The current demonstration deliberately places its data outside the program region.

## Fetch, decode, and execute

Each CPU step follows three conceptual stages:

1. **Fetch:** read the byte at the address held in `PC`, then increment `PC`.
2. **Decode:** interpret the fetched byte as an opcode.
3. **Execute:** perform the operation and, when required, fetch one additional operand byte.

The fetch operation is conceptually:

```text
address = PC
value   = memory[address]
PC      = PC + 1
```

The opcode determines whether another byte must be fetched. A one-byte instruction finishes after the opcode. A two-byte instruction fetches its operand from the following address.

### One-byte example

```asm
ADD A, B
```

is encoded as:

```text
0x20
```

The opcode itself identifies both registers, so no additional operand byte is needed.

### Two-byte example

```asm
LDI A, 0x2A
```

is encoded as two consecutive bytes:

| Address | Byte | Meaning |
| ---: | ---: | --- |
| `0x00` | `0x10` | Opcode for `LDI A`. |
| `0x01` | `0x2A` | Immediate value loaded into `A`. |

The CPU does not need a 16-bit register to execute this instruction. It first fetches `0x10`, recognizes that the opcode requires an operand, and then fetches `0x2A`. Each fetch transfers one 8-bit value.

## Instruction set

| Assembly syntax | Encoded bytes | Length | Effect |
| --- | --- | ---: | --- |
| `NOP` | `00` | 1 | Performs no state change other than normal `PC` and cycle advancement. |
| `HALT` | `01` | 1 | Sets the halted state. |
| `LDI A, imm8` | `10 imm8` | 2 | Loads an immediate byte into `A`; updates `Z`; preserves `C`. |
| `LDI B, imm8` | `11 imm8` | 2 | Loads an immediate byte into `B`; updates `Z`; preserves `C`. |
| `ADD A, B` | `20` | 1 | Stores the low eight bits of `A + B` in `A`; updates `Z` and addition carry. |
| `SUB A, B` | `21` | 1 | Stores wrapped `A - B` in `A`; updates `Z` and subtraction borrow. |
| `JZ addr8` | `30 addr8` | 2 | Loads `PC` with the absolute address when `Z` is set. |
| `JMP addr8` | `31 addr8` | 2 | Always loads `PC` with the absolute address. |
| `LDA addr8` | `40 addr8` | 2 | Loads `A` from the given memory address; updates `Z`; preserves `C`. |
| `STA addr8` | `41 addr8` | 2 | Stores `A` at the given memory address; preserves the registers and flags. |

`imm8` and `addr8` are each one byte. They may therefore represent values from `0x00` through `0xFF`.

## Flags

### Zero flag

The zero flag is set when a flag-updating instruction produces zero. It is currently updated by:

- `LDI A, imm8`;
- `LDI B, imm8`;
- `ADD A, B`;
- `SUB A, B`;
- `LDA addr8`.

`JZ` reads the zero flag but does not modify it.

### Carry flag

The carry flag has two related unsigned meanings:

- after `ADD`, it reports carry-out beyond `0xFF`;
- after `SUB`, it reports that a borrow was required because the original `A` was smaller than `B`.

For example:

```text
0xFF + 0x01 = 0x100
stored A     = 0x00
zero         = set
carry        = set
```

For subtraction:

```text
0x00 - 0x01 = 0xFF after 8-bit wrapping
zero         = clear
carry/borrow = set
```

## Program counter and cycle counter

The program counter counts **byte addresses**, not instructions. Therefore:

- a one-byte instruction normally advances `PC` by one;
- a two-byte instruction normally advances `PC` by two;
- a taken jump replaces `PC` with its target address after the target byte has been fetched.

The cycle counter follows a deliberately simplified model: every attempted instruction counts as one cycle, regardless of whether that instruction occupies one or two bytes.

Thus, an 18-byte program may execute only nine instructions and finish with:

```text
Program counter: 18
Cycle count: 9
```

This is an educational abstraction. A physical processor may require different numbers of clock cycles for different instructions and memory accesses.

## Assembly source, labels, and symbols

Assembly source is text processed by `vm8asm`, which runs as a normal program on the host computer. The virtual CPU does not parse this text.

A label declaration associates a name with the current output address:

```asm
start:
```

The colon declares the label but is not part of the stored name. A later instruction may reference it:

```asm
JMP start
```

A **symbol** is a name associated with information known by the assembler. In the current language, symbols are labels associated with byte addresses. The symbol table might contain:

```text
start       -> 0x00
memory_demo -> 0x09
```

The names may contain several characters because they are stored and processed by the host-side assembler. Only the resolved 8-bit address is written to machine code. The name itself never enters CPU memory.

Labels are case-sensitive, must begin with a letter or underscore, and may continue with letters, digits, or underscores. Instruction mnemonics and register names are reserved words and cannot be used as labels.

## Why the assembler uses two passes

A jump may reference a label that appears later in the source:

```asm
  JZ finished
  LDI A, 0xFF

finished:
  HALT
```

When the assembler first encounters `JZ finished`, the final address of `finished` is not known yet. Two passes solve this cleanly.

### Source reading and normalization

Before either pass, the source reader:

1. reads one physical line at a time;
2. rejects a source line longer than 255 characters;
3. removes text beginning with `;` because it is a comment;
4. removes surrounding whitespace;
5. ignores empty results;
6. sends each remaining statement, its physical line number, and its file path to a callback.

For example:

```asm
    LDI A, 0x2A   ; Load the initial value.
```

becomes:

```text
LDI A, 0x2A
```

The original physical line number is preserved for diagnostics.

### First pass

The first pass maintains a current program size that also acts as the address of the next emitted byte.

- A label is entered into the symbol table with the current address and emits zero bytes.
- A one-byte instruction advances the size by one.
- A two-byte instruction advances the size by two.
- Duplicate, malformed, reserved, or out-of-memory labels are rejected.
- Unknown instruction mnemonics and programs larger than memory are rejected.

Operand syntax is not fully interpreted during this pass. Only the mnemonic and encoded instruction length are required to calculate addresses.

### Literal and symbol resolution

The byte-literal parser accepts strict decimal or `0x`-prefixed hexadecimal values:

```text
42   -> 0x2A
0x2A -> 0x2A
255  -> 0xFF
```

Signs, embedded whitespace, malformed digits, and values above 255 are rejected.

The byte-operand resolver first attempts literal parsing. If the text is not a literal but is a valid symbol name, it searches the symbol table:

```text
0x80        -> literal byte 0x80
memory_demo -> symbol address 0x09
missing     -> undefined-symbol error
```

### Second pass

The second pass will read the normalized statements again, validate their complete operand syntax, resolve literals and labels, and emit the corresponding opcode and operand bytes.

For example:

```asm
JZ memory_demo
```

will become:

```text
30 09
```

After the second pass, the binary writer will store the emitted byte array in `build/demo.bin`. This stage is the next major assembler milestone and is not implemented yet.

## Complete demonstration encoding

The current Assembly demonstration is:

```asm
; Demonstrates arithmetic, branching, and memory transfer.

start:
  LDI A, 0x2A
  LDI B, 0x2A
  SUB A, B
  JZ memory_demo
  LDI A, 0xFF

memory_demo:
  LDI A, 0x5A
  STA 0x80
  LDI A, 0x00
  LDA 0x80
  HALT
```

Its address calculation and intended encoding are:

| Address | Source statement | Emitted bytes | Explanation |
| ---: | --- | --- | --- |
| `0x00` | `start:` | — | Records `start = 0x00`; emits nothing. |
| `0x00` | `LDI A, 0x2A` | `10 2A` | Occupies `0x00` and `0x01`. |
| `0x02` | `LDI B, 0x2A` | `11 2A` | Occupies `0x02` and `0x03`. |
| `0x04` | `SUB A, B` | `21` | One-byte instruction. |
| `0x05` | `JZ memory_demo` | `30 09` | Resolves `memory_demo` to `0x09`. |
| `0x07` | `LDI A, 0xFF` | `10 FF` | Skipped when the branch is taken. |
| `0x09` | `memory_demo:` | — | Records `memory_demo = 0x09`; emits nothing. |
| `0x09` | `LDI A, 0x5A` | `10 5A` | Occupies `0x09` and `0x0A`. |
| `0x0B` | `STA 0x80` | `41 80` | Stores `A` in data memory. |
| `0x0D` | `LDI A, 0x00` | `10 00` | Clears `A`. |
| `0x0F` | `LDA 0x80` | `40 80` | Reloads the stored value. |
| `0x11` | `HALT` | `01` | Halts after fetching the byte. |

The complete intended 18-byte sequence is:

```text
10 2A 11 2A 21 30 09 10 FF 10 5A 41 80 10 00 40 80 01
```

## Demonstration execution walkthrough

The CPU begins with zeroed registers, clear flags, `PC = 0x00`, and cycle count zero.

| Step | Address | Instruction | Important state after execution |
| ---: | ---: | --- | --- |
| 1 | `0x00` | `LDI A, 0x2A` | `A = 0x2A`, `Z = 0`, `PC = 0x02`, cycles = 1. |
| 2 | `0x02` | `LDI B, 0x2A` | `B = 0x2A`, `Z = 0`, `PC = 0x04`, cycles = 2. |
| 3 | `0x04` | `SUB A, B` | `A = 0x00`, `Z = 1`, no borrow, `PC = 0x05`, cycles = 3. |
| 4 | `0x05` | `JZ 0x09` | Target operand is fetched and the set zero flag changes `PC` to `0x09`; cycles = 4. |
| 5 | `0x09` | `LDI A, 0x5A` | `A = 0x5A`, `Z = 0`, `PC = 0x0B`, cycles = 5. |
| 6 | `0x0B` | `STA 0x80` | `memory[0x80] = 0x5A`, `PC = 0x0D`, cycles = 6. |
| 7 | `0x0D` | `LDI A, 0x00` | `A = 0x00`, `Z = 1`, `PC = 0x0F`, cycles = 7. |
| 8 | `0x0F` | `LDA 0x80` | `A = 0x5A`, `Z = 0`, `PC = 0x11`, cycles = 8. |
| 9 | `0x11` | `HALT` | CPU halted, `PC = 0x12` (decimal 18), cycles = 9. |

The instruction at `0x07` is never executed because `SUB A, B` produced zero and `JZ` jumped directly to `0x09`.

The final visible state is:

```text
Execution result: halted
Register A: 0x5A
Register B: 0x2A
Zero flag: clear
Carry flag: clear
Program counter: 18
Cycle count: 9
```

## Loading and bounded execution

`cpu_load_program` validates that the byte sequence fits in memory and rejects a null pointer for a nonempty program. It copies the program beginning at address zero, but does not reset the CPU automatically.

`cpu_run` receives an instruction limit. This prevents an unconditional loop such as `JMP 0x00` from running forever without returning control to the caller. Execution reports one of three outcomes:

- halted normally;
- invalid opcode;
- instruction limit reached.

An invalid opcode also halts the CPU so that execution cannot silently continue through unknown data.

## Current module responsibilities

| Module | Responsibility |
| --- | --- |
| `include/cpu.h`, `src/cpu.c` | CPU state, memory operations, fetching, decoding, execution, program loading, and bounded running. |
| `include/program.h`, `src/program.c` | Immutable descriptor and current built-in demonstration bytecode. |
| `include/cli.h`, `src/cli.c` | Simulator command parsing and help presentation. |
| `src/main.c` | High-level simulator orchestration and final state presentation. |
| `assembler/source_line.*` | Comment removal and whitespace normalization. |
| `assembler/source_reader.*` | Bounded file reading and callback delivery with source locations. |
| `assembler/symbol_table.*` | Mapping symbol names to 8-bit addresses. |
| `assembler/first_pass.*` | Label collection, instruction-size calculation, and memory-capacity validation. |
| `assembler/byte_literal.*` | Strict conversion of decimal and hexadecimal text to `uint8_t`. |
| `assembler/byte_operand.*` | Resolution of a literal or symbol into one byte. |
| `assembler/main.c` | Assembler argument handling and first-pass orchestration. |
| `tests/` | Independent behavioral tests for the CPU, CLI, program, and assembler components. |

Keeping `main.c` focused on orchestration makes reusable behavior independently testable.

## Build and execution commands

From the repository root inside Ubuntu:

```bash
make
```

Builds the simulator.

```bash
make run
```

Builds and executes the simulator with the built-in bytecode demonstration.

```bash
make help
```

Displays simulator commands and the instruction reference.

```bash
make test
```

Builds and runs all automated tests.

```bash
make assembler
```

Builds the standalone `build/vm8asm` executable.

```bash
make assemble
```

Builds `vm8asm` when necessary and analyzes `programs/demo.asm`. Until the second pass and binary writer are completed, it reports the first-pass result without creating `build/demo.bin`.

## Key ideas to retain

- Eight-bit values are fetched and processed one byte at a time; an instruction may contain multiple bytes.
- The opcode tells the CPU how many additional bytes to fetch and how to interpret them.
- Labels and mnemonics belong to the assembler, not to the CPU.
- A label consumes no program memory; it names the current byte address.
- The first pass discovers addresses, and the second pass replaces symbolic references with numeric bytes.
- The CPU ultimately executes only the generated byte sequence.
- `PC` measures byte addresses, while the simplified cycle counter measures attempted instructions.
- Unified memory permits both code and data access, so stores must use addresses carefully.

