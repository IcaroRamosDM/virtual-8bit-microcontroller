# How the Virtual 8-bit Microcontroller Works

[Read this documentation in Brazilian Portuguese.](HOW_IT_WORKS.pt-BR.md)

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

The CPU simulator is operational. It can run the 18-byte built-in demonstration stored in `src/program.c` or a compatible raw binary selected on the command line. The current Assembly demonstration generates a 19-byte binary because it includes one embedded data byte after `HALT`. Either source can run normally or with a human-readable instruction trace.

The CPU now has a 16-byte downward-growing stack. `PUSH A` and `POP A` transfer data through it, while `CALL addr8` and `RET` use it to save and restore 8-bit return addresses. Stack overflow and underflow are explicit execution errors shared by both kinds of operation.

The CPU also has one host-controlled input port at address `0xEE` and one output latch at `0xEF`. They are accessed through normal `LDA` and `STA` instructions because the CPU decodes those addresses as memory-mapped I/O.

The shared `instruction_set` module owns the `Opcode` definitions and a read-only metadata table that maps every supported opcode byte to its Assembly mnemonic. A lookup receives a raw `uint8_t` because memory may contain any byte; it returns a metadata pointer for a recognized opcode or a null pointer for an unknown value.

The assembler currently implements:

- command-line argument validation;
- bounded source-file reading;
- comment and surrounding-whitespace removal;
- delivery of normalized statements through a callback;
- a symbol table shared by labels and named constants;
- a first pass that records labels and constants and calculates the size of instructions and raw data;
- decimal and hexadecimal 8-bit literal parsing;
- resolution of an operand as either a literal or a symbol;
- a parser that separates instruction or directive names and operands;
- an instruction encoder that validates instruction semantics and emits opcodes and operands;
- a second pass that skips `.EQU`, resolves `.BYTE` operands, encodes instructions, and accumulates the complete program byte sequence;
- a binary writer that stores the raw bytes in the requested output file.

`make assemble` translates `programs/demo.asm` into the 19-byte `build/demo.bin` file. `make inspect` performs that assembly and then displays the generated size and raw bytes. `make run` executes the 18-byte built-in bytecode from `src/program.c`, while `make run-bin` assembles, loads, and executes `build/demo.bin`. The two demonstrations are behaviorally equivalent rather than byte-for-byte identical. `make trace` and `make trace-bin` select the same two program sources while exposing every executed instruction and its resulting CPU state.

## What “8-bit” means

In this project, 8-bit describes the natural data width of the CPU:

- register `A` stores one 8-bit value;
- register `B` stores one 8-bit value;
- each memory location stores one 8-bit value;
- the program counter stores one 8-bit address;
- the stack pointer stores one 8-bit stack address or the empty-stack sentinel;
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
| Stack pointer (`SP`) | 8 bits | Address of the newest stack byte, or `0x00` when the stack is empty. |
| Input port | 8 bits | Value supplied by the host and read by VM8 software at `0xEE`. |
| Output latch | 8 bits | Value read or written by VM8 software at `0xEF` and observed by the host. |
| Zero flag (`Z`) | Boolean | Indicates that the most recent flag-updating result was zero. |
| Carry flag (`C`) | Boolean | Indicates addition carry-out or subtraction borrow. |
| Halted state | Boolean | Prevents further execution after a halt, invalid opcode, or stack error. |
| Cycle counter | 64 bits | Counts attempted instructions in the simplified timing model. |
| Memory array | 256 bytes | Backs ordinary code/data and stack storage; mapped I/O is decoded before ordinary array access. |

The normal initial state is completely zero-initialized:

```c
Cpu cpu = {0};
```

The program is then copied into memory beginning at address `0x00`. Although the complete address space contains 256 locations, a loaded program may occupy at most the first 238 bytes. Addresses `0xEE` and `0xEF` are reserved for I/O, and the final 16 addresses are reserved for the stack.

## Unified code and data memory

The project uses one unified 8-bit address space. Instructions and ordinary data share the memory array, while the CPU intercepts the two I/O addresses before ordinary memory access. Stack data uses the final part of the same array.

| Address range | Size | Use |
| --- | ---: | --- |
| `0x00` through `0xED` | 238 bytes | Loaded program and ordinary program-selected data. |
| `0xEE` | 1 byte | Read-only virtual input port. |
| `0xEF` | 1 byte | Readable and writable virtual output latch. |
| `0xF0` through `0xFF` | 16 bytes | CPU-managed stack. |

For example, the demonstration program occupies addresses `0x00` through `0x11`, while it uses address `0x80` to store data. The instruction:

```asm
STA 0x80
```

writes register `A` into memory location `0x80`.

Because code and data share the same array, a store directed at a program address could overwrite an instruction. The current demonstration deliberately places its writable data outside its instruction bytes. Software can still address the stack region with ordinary memory instructions, but doing so can corrupt stack contents. The assembler and program loader guarantee that the loaded binary itself occupies only `0x00` through `0xED`.

## Memory-mapped input and output

Memory-mapped I/O means that the CPU uses normal memory instructions for peripheral access. The address decoder decides whether an address refers to the memory array or to a device register:

| Address | Read behavior | Write behavior |
| --- | --- | --- |
| `0xEE` | Returns the current host-supplied input value. | Ignored, because the port is input-only from VM8 software's perspective. |
| `0xEF` | Returns the current output-latch value. | Replaces the output-latch value with the written byte. |
| Any other address | Reads the corresponding memory-array byte. | Writes the corresponding memory-array byte. |

The input and output values are fields in the `Cpu` state rather than bytes stored at those two array indexes. The host calls `cpu_set_input_port` before execution. VM8 software reads the input through `cpu_read_memory`, normally by executing `LDA 0xEE`. Software changes the output by executing `STA 0xEF`, and the host observes it through `cpu_get_output_port`. A complete CPU reset clears both port values.

This program copies the input port to the output latch:

```asm
.EQU INPUT_PORT, 0xEE
.EQU OUTPUT_PORT, 0xEF

start:
  LDA INPUT_PORT
  STA OUTPUT_PORT
  HALT
```

The assembler resolves the constants and emits five bytes:

```text
40 ee 41 ef 01
```

The same machine-code program can be created directly for a quick experiment:

```bash
make
printf '\x40\xEE\x41\xEF\x01' > build/io-demo.bin
./build/vm8 run build/io-demo.bin --input 0xA5
```

The relevant final state is:

```text
Register A: 0xA5
Input port: 0xA5
Output port: 0xA5
Program counter: 5
Cycle count: 3
```

The same value may be written in decimal, and tracing reveals exactly when the output changes:

```bash
./build/vm8 trace build/io-demo.bin --input 165
```

```text
Execution trace:
  ADDR=0x00 OP=0x40 MNEMONIC=LDA A=0xA5 B=0x00 SP=0x00 IN=0xA5 OUT=0x00 Z=0 C=0 NEXT=0x02 CYCLES=1 RESULT=ok
  ADDR=0x02 OP=0x41 MNEMONIC=STA A=0xA5 B=0x00 SP=0x00 IN=0xA5 OUT=0xA5 Z=0 C=0 NEXT=0x04 CYCLES=2 RESULT=ok
  ADDR=0x04 OP=0x01 MNEMONIC=HALT A=0xA5 B=0x00 SP=0x00 IN=0xA5 OUT=0xA5 Z=0 C=0 NEXT=0x05 CYCLES=3 RESULT=halted
```

The CLI accepts decimal or `0x`-prefixed hexadecimal input from 0 through 255. Signs, leading whitespace, trailing text, missing values, and larger values are rejected. When present, `--input <byte>` must be the final option. Without it, the input defaults to `0x00`.

The Make targets expose the same option through `INPUT_VALUE`, for example `make run INPUT_VALUE=0xA5`. The built-in demonstration does not read `0xEE`, so its output latch remains zero even though the selected input value is still visible in the final state and trace.

## Stack, `PUSH`, and `POP`

A stack is a last-in, first-out storage area. The newest byte pushed is the first byte returned by a pop. VM8 reserves addresses `0xF0` through `0xFF` for a stack that grows downward toward smaller addresses.

`SP = 0x00` represents an empty stack. This is a sentinel value, not an address currently occupied by stack data. It lets the normal zero initialization `Cpu cpu = {0};` create a valid empty stack without requiring a separate initialization assignment.

`PUSH A` works as follows:

1. If the stack is empty, set `SP` to `0xFF`.
2. Otherwise, decrement `SP` before writing.
3. Store register `A` at `memory[SP]`.

`POP A` performs the inverse operation:

1. Read `memory[SP]` into register `A`.
2. If the removed byte was at `0xFF`, set `SP` back to the empty sentinel `0x00`.
3. Otherwise, increment `SP` toward `0xFF`.

For example:

| Operation | Resulting `SP` | Relevant memory or result |
| --- | ---: | --- |
| Initial empty state | `0x00` | No stack byte is active. |
| Push `0xA5` | `0xFF` | `memory[0xFF] = 0xA5`. |
| Push `0x5A` | `0xFE` | `memory[0xFE] = 0x5A`; `0xA5` remains below it at `0xFF`. |
| Pop | `0xFF` | `A = 0x5A`. |
| Pop again | `0x00` | `A = 0xA5`; the stack is empty again. |

After 16 pushes, every address from `0xFF` down through `0xF0` is occupied and `SP` equals `0xF0`. Another `PUSH A` reports stack overflow. A `POP A` while `SP` is `0x00` reports stack underflow. Either error halts execution and produces a distinct step and run result without changing registers, flags, stack memory, or `SP`; the attempted opcode fetch and cycle count have already occurred.

`PUSH A` preserves both registers and both flags. `POP A` replaces `A`, updates `Z` according to whether the popped byte is zero, preserves `B` and `C`, and does not erase the byte left in memory. Advancing `SP` is what makes that location no longer part of the active stack.

## Subroutines, `CALL`, and `RET`

A subroutine is a reusable block of instructions that temporarily receives control and then returns to its caller. A normal jump changes `PC` but does not remember where execution came from. `CALL addr8` performs both jobs: it saves the return address on the stack and then jumps to the subroutine. `RET` removes that address from the stack and places it back in `PC`.

`CALL addr8` occupies two bytes: opcode `0x52` followed by one absolute address byte. Its execution order is important:

1. The normal instruction fetch reads the `CALL` opcode and advances `PC`.
2. A second fetch reads `addr8` and advances `PC` again.
3. The already-advanced `PC`, which now identifies the instruction after `CALL`, is pushed as the return address.
4. The target address replaces `PC`, transferring execution to the subroutine.

The return address is therefore calculated at run time; it is not another byte encoded in the program. `RET` occupies one byte. After its opcode is fetched, it pops the newest stack byte directly into `PC`. Both instructions preserve registers `A` and `B` and flags `Z` and `C`.

Consider this nested-call program, whose labels are annotated with their resulting byte addresses:

```asm
start:           ; address 0x00
  CALL first     ; bytes: 52 03
  HALT           ; address 0x02

first:           ; address 0x03
  CALL second    ; bytes: 52 06
  RET            ; address 0x05

second:          ; address 0x06
  RET
```

The assembler resolves the labels and emits only these seven bytes:

```text
52 03 01 52 06 53 53
```

The execution sequence is:

1. `CALL first` pushes `0x02` at `memory[0xFF]`, leaves `SP = 0xFF`, and sets `PC = 0x03`.
2. `CALL second` pushes `0x05` at `memory[0xFE]`, leaves `SP = 0xFE`, and sets `PC = 0x06`.
3. The `RET` at `0x06` pops `0x05`, leaves `SP = 0xFF`, and resumes the first subroutine at `0x05`.
4. The `RET` at `0x05` pops `0x02`, restores the empty sentinel `SP = 0x00`, and resumes the caller at `0x02`.
5. `HALT` executes at `0x02`. Five instructions were attempted, so the simplified cycle count is five.

The last return address pushed is the first one recovered, which is exactly the last-in, first-out behavior required for nested calls.

Return addresses and values pushed by `PUSH A` share the same 16-byte capacity. Sixteen calls can be nested only when no other stack entries are active. Each active `PUSH A` consumes one location that would otherwise be available to a call. Software must also keep operations balanced: an unexpected `POP A`, an extra `PUSH A`, or a `RET` in the wrong place can consume the wrong byte because the stack stores bytes without recording whether each byte represents data or a return address.

A `CALL` on a full stack reports stack overflow after its opcode and target operand have been fetched, so `PC` already contains the would-be return address; the jump is not taken. A `RET` on an empty stack reports stack underflow after its opcode fetch. Either error halts execution, counts the attempted instruction, and preserves registers, flags, stack memory, and `SP`.

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
| `AND A, B` | `22` | 1 | Stores the bitwise AND in `A`; updates `Z`; clears `C`. |
| `OR A, B` | `23` | 1 | Stores the bitwise OR in `A`; updates `Z`; clears `C`. |
| `XOR A, B` | `24` | 1 | Stores the bitwise exclusive OR in `A`; updates `Z`; clears `C`. |
| `NOT A` | `25` | 1 | Inverts every bit in `A`; updates `Z`; clears `C`. |
| `SHL A` | `26` | 1 | Shifts `A` left with zero fill; updates `Z`; stores the original bit 7 in `C`. |
| `SHR A` | `27` | 1 | Shifts `A` right with zero fill; updates `Z`; stores the original bit 0 in `C`. |
| `CMP A, B` | `28` | 1 | Compares unsigned `A` with `B`; updates `Z` and borrow; preserves both registers. |
| `JZ addr8` | `30 addr8` | 2 | Loads `PC` with the absolute address when `Z` is set. |
| `JNZ addr8` | `32 addr8` | 2 | Loads `PC` with the absolute address when `Z` is clear. |
| `JC addr8` | `33 addr8` | 2 | Loads `PC` with the absolute address when `C` is set. |
| `JMP addr8` | `31 addr8` | 2 | Always loads `PC` with the absolute address. |
| `LDA addr8` | `40 addr8` | 2 | Loads `A` from the given memory address; updates `Z`; preserves `C`. |
| `STA addr8` | `41 addr8` | 2 | Stores `A` at the given memory address; preserves the registers and flags. |
| `PUSH A` | `50` | 1 | Pushes `A` onto the stack; preserves registers and flags. |
| `POP A` | `51` | 1 | Pops the newest stack byte into `A`; updates `Z`; preserves `C`. |
| `CALL addr8` | `52 addr8` | 2 | Pushes the address after the operand, then loads `PC` with the absolute target; preserves registers and flags. |
| `RET` | `53` | 1 | Pops the newest stack byte directly into `PC`; preserves registers and flags. |

`imm8` and `addr8` are each one byte. They may therefore represent values from `0x00` through `0xFF`.
`PUSH A`, `POP A`, and `RET` require no encoded operand byte because their opcodes identify their complete operations. `CALL` requires one `addr8` operand that may be written as a literal, constant, or label.

## Logical and bit operations

`AND`, `OR`, and `XOR` compare corresponding bits of `A` and `B`. The result replaces `A`, while `B` remains unchanged. `NOT` operates only on `A` and flips each zero to one and each one to zero. These four instructions clear `C` and set `Z` when the result stored in `A` is zero.

For example, with `A = 0xCA` and `B = 0xAC`:

```text
AND: 11001010 & 10101100 = 10001000 = 0x88
OR:  11001010 | 10101100 = 11101110 = 0xEE
XOR: 11001010 ^ 10101100 = 01100110 = 0x66
```

For an 8-bit inversion:

```text
NOT: ~00001111 = 11110000
     ~0x0F      = 0xF0
```

`SHL` and `SHR` move every bit in `A` by one position and insert zero into the newly opened position. The bit that leaves the register is not silently lost: it is copied into `C`.

```text
SHL: A = 10000001 -> A = 00000010, C = 1
SHR: A = 10000001 -> A = 01000000, C = 1
```

The equivalent Assembly example for the left shift is:

```asm
LDI A, 0x81
SHL A
```

After `SHL A`, register `A` contains `0x02`, carry is set because the original bit 7 was one, and zero is clear because the result is not zero. The binary notation above is explanatory; the current assembler accepts decimal and hexadecimal byte literals, not `0b` binary literals.

## Flags

### Zero flag

The zero flag is set when a flag-updating instruction produces zero. It is currently updated by:

- `LDI A, imm8`;
- `LDI B, imm8`;
- `ADD A, B`;
- `SUB A, B`;
- `AND A, B`;
- `OR A, B`;
- `XOR A, B`;
- `NOT A`;
- `SHL A`;
- `SHR A`;
- `CMP A, B`;
- `LDA addr8`;
- `POP A`.

`JZ` and `JNZ` read the zero flag but do not modify it.

### Carry flag

The carry flag records information that does not fit in the 8-bit result:

- after `ADD`, it reports carry-out beyond `0xFF`;
- after `SUB`, it reports that a borrow was required because the original `A` was smaller than `B`;
- after `CMP`, it reports that the unsigned value in `A` is smaller than the value in `B`;
- after `SHL`, it receives the original bit 7;
- after `SHR`, it receives the original bit 0;
- after `AND`, `OR`, `XOR`, or `NOT`, it is cleared.

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

`JC` reads the carry flag but does not modify it. `PUSH A` preserves both flags, while `POP A` updates zero and preserves carry.

## Comparison and conditional branches

`CMP A, B` behaves like an unsigned subtraction used only for decision-making. Internally, the CPU calculates the wrapped 8-bit value of `A - B` and the associated borrow, uses them to update `Z` and `C`, and then discards the subtraction result. Registers `A` and `B` remain unchanged.

| Relationship | `Z` after `CMP` | `C` after `CMP` | Meaning |
| --- | ---: | ---: | --- |
| `A == B` | 1 | 0 | The values are equal. |
| `A > B` | 0 | 0 | Unsigned `A` is greater than `B`. |
| `A < B` | 0 | 1 | The subtraction would need a borrow. |

This makes the conditional branches useful immediately after a comparison:

- `JZ` branches when the compared values are equal;
- `JNZ` branches when the compared values are different;
- `JC` branches when unsigned `A` is less than `B`.

For example, equality can be tested without destroying either operand:

```asm
LDI A, 0x2A
LDI B, 0x2A
CMP A, B
JZ equal_values
```

`JNZ` can also repeat a loop while an arithmetic result remains nonzero:

```asm
LDI A, 3
LDI B, 1

loop:
  SUB A, B
  JNZ loop

HALT
```

The loop executes `SUB` three times. The first two results leave `Z` clear, so `JNZ` returns to `loop`. The third result is zero, so execution continues to `HALT`.

An unsigned less-than decision uses the borrow recorded in `C`:

```asm
LDI A, 0x10
LDI B, 0x20
CMP A, B
JC a_is_lower
```

Every conditional jump occupies two bytes and always fetches its address operand. If the condition is true, the target replaces `PC`; otherwise `PC` already points to the following instruction. The jump itself preserves both registers and both flags. Because several instructions can update carry, the meaning of `JC` depends on the most recent flag-producing instruction; its less-than interpretation specifically follows `CMP`.

## Program counter and cycle counter

The program counter counts **byte addresses**, not instructions. Therefore:

- a one-byte instruction normally advances `PC` by one;
- a two-byte instruction normally advances `PC` by two;
- a taken jump replaces `PC` with its target address after the target byte has been fetched.

The cycle counter follows a deliberately simplified model: every attempted instruction counts as one cycle, regardless of whether that instruction occupies one or two bytes.

Thus, program size and executed instruction count are different measurements. The current 18-byte built-in demonstration and 19-byte assembled demonstration both execute only nine instructions and finish with:

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

A **symbol** is a name associated with an 8-bit value known by the assembler. In the current language, a symbol may be either a label address or a named constant. Both kinds share one symbol table and one namespace. The table built for the current demonstration includes entries such as:

```text
start            -> 0x00  (label address)
memory_demo      -> 0x09  (label address)
initial_data     -> 0x12  (label address)
COMPARISON_VALUE -> 0x2A  (named constant)
DATA_ADDRESS     -> 0x80  (named constant)
```

The names may contain several characters because they are stored and processed by the host-side assembler. When a symbol is used as an operand, only its resolved 8-bit value is written to machine code. The name itself never enters CPU memory.

Labels and constants are case-sensitive, must begin with a letter or underscore, and may continue with letters, digits, or underscores. Instruction mnemonics and register names are reserved and cannot be used as symbols. A label and a constant also cannot reuse the same name because they share one namespace.

### Named constants and raw data

`.EQU NAME, value` associates `NAME` with a decimal or `0x` hexadecimal literal from 0 through 255. It emits no byte:

```asm
.EQU STORED_VALUE, 0x5A
```

`.BYTE value` emits exactly one raw byte. Its operand may be a literal, a named constant, or a label. This allows data to live next to executable instructions:

```asm
.EQU STORED_VALUE, 0x5A

  LDA initial_data
  HALT

initial_data:
  .BYTE STORED_VALUE
```

This example generates `40 03 01 5A`. The `LDA` operand is `0x03`, the address recorded for `initial_data`; the byte at that address is `0x5A`. Neither symbolic name is stored in the binary.

## Host-side assembly and limited-memory machines

`vm8asm` runs on the Ubuntu host, so its executable code, source-text buffers, and symbol table do not consume any of the VM8 CPU's 256-byte memory. Only the generated machine-code bytes are copied into the virtual CPU.

Writing those bytes manually in hexadecimal would not make the final program smaller. For example, manually entering `10 2A` produces the same two bytes as assembling `LDI A, 0x2A`; hexadecimal is only a readable notation for the bit patterns.

On an isolated historical machine without a separate development computer, programmers had several options:

- translate instructions manually and enter machine-code values through switches, cards, or tape;
- enter a very small bootstrap loader that could load a larger tool;
- load an assembler temporarily into the same memory used later by the program;
- keep a small monitor or assembler permanently in a separate ROM region.

When an assembler occupied the machine's own limited memory, source and output could be streamed through external media. After assembly, the assembler could be overwritten and its memory reused by the generated program. A multi-pass assembler traded additional input passes and time for a smaller in-memory working set.

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
- `.EQU` enters its name and literal value into the same symbol table and emits zero bytes.
- `.BYTE` advances the size by one because it will emit one byte during the second pass.
- A one-byte instruction advances the size by one.
- A two-byte instruction advances the size by two.
- Duplicate symbol names are rejected even when one declaration is a label and the other is a constant.
- Malformed or reserved names, invalid directives, unknown instruction mnemonics, and programs larger than memory are rejected.

The `.EQU` literal must be validated during this pass because its value is stored immediately. Instruction and `.BYTE` symbolic operands can wait for the second pass; the first pass needs only their output sizes to continue calculating addresses.

### Literal and symbol resolution

The byte-literal parser accepts strict decimal or `0x`-prefixed hexadecimal values:

```text
42   -> 0x2A
0x2A -> 0x2A
255  -> 0xFF
```

Signs, embedded whitespace, malformed digits, and values above 255 are rejected.

The byte-operand resolver first attempts literal parsing. If the text is not a literal but is a valid symbol name, it searches the shared symbol table. The same resolver can therefore return a literal, a constant value, or a label address:

```text
0x80             -> literal byte 0x80
DATA_ADDRESS     -> constant value 0x80
memory_demo      -> label address 0x09
initial_data     -> label address 0x12
missing          -> undefined-symbol error
```

### Second pass

The parser separates each normalized statement into a leading name and as many as two operands. It validates structural syntax such as whitespace, commas, missing operands, and excessive operands, but does not decide whether an instruction, directive, or register is supported.

The instruction encoder then validates the meaning of the parsed fields. It recognizes the current instruction set, checks operand counts and register order, resolves byte literals or symbols, and emits a one-byte or two-byte encoded instruction. `AND`, `OR`, `XOR`, and `CMP` require the exact register pair `A, B`; `NOT`, `SHL`, and `SHR` require the single register `A`; and `PUSH` and `POP` require `A`. `JZ`, `JNZ`, `JC`, `JMP`, `LDA`, `STA`, and `CALL` accept one literal or symbolic byte address. `NOP`, `HALT`, and `RET` accept no operands. Failed encoding leaves the caller's output object unchanged.

The second pass reads the normalized statements again and handles each kind explicitly:

- label declarations are ignored because their addresses are already in the symbol table;
- `.EQU` is skipped because it has already defined a value and emits no bytes;
- `.BYTE` resolves its one operand and appends exactly one byte;
- ordinary instructions pass through semantic validation and encoding.

Every emitted byte is appended to a bounded program buffer. The pass reports diagnostics with the original file path and line number and rejects any write that would exceed the supplied output capacity. Only encoded CPU instructions increment the instruction count, so `.BYTE` changes the byte count but not the instruction count.

For example:

```asm
JZ memory_demo
```

becomes:

```text
30 09
```

After both passes agree on the 19-byte program size, the binary writer opens the requested path in binary mode, writes exactly that byte count, and verifies both the write and final file close. `make assemble` therefore creates `build/demo.bin` as raw machine code.

## Complete demonstration encoding

The current Assembly demonstration is:

```asm
; Demonstrates constants, embedded data, arithmetic, branching, and memory transfer.

.EQU COMPARISON_VALUE, 0x2A
.EQU FALLTHROUGH_VALUE, 0xFF
.EQU STORED_VALUE, 0x5A
.EQU CLEARED_VALUE, 0x00
.EQU DATA_ADDRESS, 0x80

start:
  LDI A, COMPARISON_VALUE
  LDI B, COMPARISON_VALUE
  SUB A, B
  JZ memory_demo
  LDI A, FALLTHROUGH_VALUE

memory_demo:
  LDA initial_data
  STA DATA_ADDRESS
  LDI A, CLEARED_VALUE
  LDA DATA_ADDRESS
  HALT

initial_data:
  .BYTE STORED_VALUE
```

Its address calculation and generated encoding are:

| Address | Source statement | Emitted bytes | Explanation |
| ---: | --- | --- | --- |
| — | `.EQU COMPARISON_VALUE, 0x2A` | — | Defines a constant; emits nothing. |
| — | `.EQU FALLTHROUGH_VALUE, 0xFF` | — | Defines a constant; emits nothing. |
| — | `.EQU STORED_VALUE, 0x5A` | — | Defines a constant; emits nothing. |
| — | `.EQU CLEARED_VALUE, 0x00` | — | Defines a constant; emits nothing. |
| — | `.EQU DATA_ADDRESS, 0x80` | — | Defines a constant; emits nothing. |
| `0x00` | `start:` | — | Records `start = 0x00`; emits nothing. |
| `0x00` | `LDI A, COMPARISON_VALUE` | `10 2A` | Resolves the constant and occupies `0x00` and `0x01`. |
| `0x02` | `LDI B, COMPARISON_VALUE` | `11 2A` | Resolves the same constant and occupies `0x02` and `0x03`. |
| `0x04` | `SUB A, B` | `21` | One-byte instruction. |
| `0x05` | `JZ memory_demo` | `30 09` | Resolves `memory_demo` to `0x09`. |
| `0x07` | `LDI A, FALLTHROUGH_VALUE` | `10 FF` | Resolves the constant; skipped when the branch is taken. |
| `0x09` | `memory_demo:` | — | Records `memory_demo = 0x09`; emits nothing. |
| `0x09` | `LDA initial_data` | `40 12` | Resolves the label to `0x12` and loads its byte. |
| `0x0B` | `STA DATA_ADDRESS` | `41 80` | Resolves the constant and stores `A` at `0x80`. |
| `0x0D` | `LDI A, CLEARED_VALUE` | `10 00` | Resolves the constant and clears `A`. |
| `0x0F` | `LDA DATA_ADDRESS` | `40 80` | Resolves the constant and reloads the stored value. |
| `0x11` | `HALT` | `01` | Halts after fetching the byte. |
| `0x12` | `initial_data:` | — | Records the address of the embedded data; emits nothing. |
| `0x12` | `.BYTE STORED_VALUE` | `5A` | Resolves the constant and emits one data byte. |

The complete generated 19-byte sequence is:

```text
10 2A 11 2A 21 30 09 10 FF 40 12 41 80 10 00 40 80 01 5A
```

The first 18 bytes contain executable code through `HALT`. The final byte at address `0x12` is data. It belongs to the loaded program image but is read by `LDA`, not fetched as an opcode.

## Demonstration execution walkthrough

The CPU begins with zeroed registers, clear flags, `PC = 0x00`, and cycle count zero.

| Step | Address | Instruction | Important state after execution |
| ---: | ---: | --- | --- |
| 1 | `0x00` | `LDI A, 0x2A` | `A = 0x2A`, `Z = 0`, `PC = 0x02`, cycles = 1. |
| 2 | `0x02` | `LDI B, 0x2A` | `B = 0x2A`, `Z = 0`, `PC = 0x04`, cycles = 2. |
| 3 | `0x04` | `SUB A, B` | `A = 0x00`, `Z = 1`, no borrow, `PC = 0x05`, cycles = 3. |
| 4 | `0x05` | `JZ 0x09` | Target operand is fetched and the set zero flag changes `PC` to `0x09`; cycles = 4. |
| 5 | `0x09` | `LDA 0x12` | Reads the embedded byte, so `A = 0x5A`, `Z = 0`, `PC = 0x0B`, cycles = 5. |
| 6 | `0x0B` | `STA 0x80` | `memory[0x80] = 0x5A`, `PC = 0x0D`, cycles = 6. |
| 7 | `0x0D` | `LDI A, 0x00` | `A = 0x00`, `Z = 1`, `PC = 0x0F`, cycles = 7. |
| 8 | `0x0F` | `LDA 0x80` | `A = 0x5A`, `Z = 0`, `PC = 0x11`, cycles = 8. |
| 9 | `0x11` | `HALT` | CPU halted, `PC = 0x12` (decimal 18), cycles = 9. |

The instruction at `0x07` is never executed because `SUB A, B` produced zero and `JZ` jumped directly to `0x09`.

The data byte at `0x12` is never executed because `HALT` stops the CPU after advancing `PC` from `0x11` to `0x12`. Its address is nevertheless valid for the earlier `LDA 0x12` instruction.

The final visible state is:

```text
Execution result: halted
Register A: 0x5A
Register B: 0x2A
Stack pointer: 0x00
Input port: 0x00
Output port: 0x00
Zero flag: clear
Carry flag: clear
Program counter: 18
Cycle count: 9
```

## Loading and bounded execution

`cpu_load_program` validates that the byte sequence fits in the 238-byte program region and rejects a null pointer for a nonempty program. It copies the program beginning at address zero, but does not reset the CPU automatically or write into the reserved I/O and stack regions.

`cpu_run` receives an instruction limit. This prevents an unconditional loop such as `JMP 0x00` from running forever without returning control to the caller. Execution reports one of five outcomes:

- halted normally;
- invalid opcode;
- instruction limit reached;
- stack overflow;
- stack underflow.

An invalid opcode or stack error also halts the CPU so that execution cannot silently continue after an invalid state transition.

## Instruction observer and execution trace

`cpu_run_with_observer` separates instruction execution from anything that watches that execution. It accepts the same CPU and instruction limit as `cpu_run`, plus two optional values:

- a `CpuStepObserver` callback;
- an opaque context pointer passed unchanged to that callback.

Before calling `cpu_step`, the run loop records the current program-counter value and reads the opcode stored there. After `cpu_step` finishes, the observer receives that original instruction address, the opcode, the updated CPU state, the step result, and the context pointer. This timing is important: `ADDR` and `OP` identify the instruction that just ran, while the registers, flags, `NEXT`, and `CYCLES` describe the state after it ran.

The original `cpu_run` function remains as a convenience wrapper. It calls `cpu_run_with_observer` with null observer and context pointers, so existing callers retain the same behavior without producing trace output.

The trace module supplies an observer that interprets its context as a `FILE *` and writes one line per attempted instruction. For example, the first instruction of the demonstration produces:

```text
Execution trace:
  ADDR=0x00 OP=0x10 MNEMONIC=LDI A=0x2A B=0x00 SP=0x00 IN=0x00 OUT=0x00 Z=0 C=0 NEXT=0x02 CYCLES=1 RESULT=ok
```

The fields mean:

- `ADDR`: address from which the opcode was fetched;
- `OP`: raw opcode byte;
- `MNEMONIC`: operation name obtained from the shared instruction-set metadata, or `UNKNOWN` when no opcode matches;
- `A` and `B`: register values after execution;
- `SP`: stack pointer after execution;
- `IN`: memory-mapped input-port value after execution;
- `OUT`: memory-mapped output-latch value after execution;
- `Z` and `C`: zero and carry flags after execution;
- `NEXT`: program counter after execution, including any taken jump;
- `CYCLES`: total attempted-instruction count after this step;
- `RESULT`: `ok`, `halted`, `invalid-opcode`, `stack-overflow`, or `stack-underflow`.

The final demonstration instruction is therefore shown as:

```text
  ADDR=0x11 OP=0x01 MNEMONIC=HALT A=0x5A B=0x2A SP=0x00 IN=0x00 OUT=0x00 Z=0 C=0 NEXT=0x12 CYCLES=9 RESULT=halted
```

This observer design keeps the CPU independent of presentation. A future debugger, logger, or graphical interface can supply a different callback without putting terminal-output code inside `cpu.c`.

## Reading and executing an external binary

The simulator-side binary reader opens the selected file in binary mode and reads into a caller-provided buffer. `main.c` supplies a buffer whose capacity is exactly `CPU_PROGRAM_MEMORY_SIZE`, so the file reader cannot write beyond the virtual machine's 238-byte program region.

After filling the buffer, the reader attempts to fetch one additional byte. This extra read distinguishes two cases that would otherwise both produce a full buffer:

- if the extra read reaches end-of-file, the program contains exactly 238 bytes and is valid;
- if another byte exists, the file is too large and is rejected.

The reader reports the number of bytes actually read only after the read and file close both succeed. An empty file is a valid binary file from the reader's narrow I/O perspective, but `main.c` rejects it as an executable program. This separation keeps file-format transport separate from simulator policy.

The CLI exposes two program sources and an optional trace for either source:

```text
make run
  -> built-in byte array from src/program.c
  -> input defaults to 0x00 or comes from INPUT_VALUE

make trace
  -> built-in byte array from src/program.c
  -> input defaults to 0x00 or comes from INPUT_VALUE
  -> execution trace enabled

make run-bin
  -> programs/demo.asm
  -> vm8asm
  -> build/demo.bin
  -> binary_reader_read
  -> cpu_load_program
  -> host input applied to the CPU
  -> cpu_run_with_observer

make trace-bin
  -> same assembled binary path
  -> execution trace enabled
```

The direct forms `./build/vm8 run <program.bin>` and `./build/vm8 trace <program.bin>` use the same external-binary path without first invoking the assembler. Both accept a final `--input <byte>` option. The simulator does not know whether the file came from `vm8asm`, another tool, or manual byte entry; it sees only the bytes. The difference is whether `main.c` gives the CPU run loop a trace observer.

The process-level Bash test exercises this public interface instead of calling C functions directly. It verifies normal external-binary execution, built-in tracing, external-binary tracing, input-to-output transfer, both numeric input forms, and five expected failures: an invalid input value, a missing file, an empty file, a 239-byte file, and a file containing the invalid opcode `0xFF`. Each failure must return a nonzero process status and place the expected diagnostic on `stderr`; successful execution must place the expected CPU state or trace entry on `stdout`.

## Current module responsibilities

| Module | Responsibility |
| --- | --- |
| `include/instruction_set.h`, `src/instruction_set.c` | Shared opcode definitions and read-only lookup from a raw opcode byte to instruction metadata. |
| `include/cpu.h`, `src/cpu.c` | CPU state, ordinary memory, memory-mapped I/O, stack operations, fetching, decoding, execution, bounded program loading, bounded running, and optional per-step observer delivery. |
| `include/cpu_trace.h`, `src/cpu_trace.c` | Human-readable formatting of post-instruction CPU snapshots, including `SP`, `IN`, `OUT`, and stack errors. |
| `include/program.h`, `src/program.c` | Immutable descriptor and current built-in demonstration bytecode. |
| `include/binary_reader.h`, `src/binary_reader.c` | Bounded raw-binary input with open, read, size, and close validation. |
| `include/cli.h`, `src/cli.c` | Selection of built-in or external-binary execution, strict virtual-input parsing, optional tracing, and help presentation. |
| `src/main.c` | Program-source selection, host-input application, optional observer wiring, high-level simulator orchestration, and final state presentation. |
| `assembler/source_line.*` | Comment removal and whitespace normalization. |
| `assembler/source_reader.*` | Bounded file reading and callback delivery with source locations. |
| `assembler/symbol_table.*` | Mapping symbol names to 8-bit label addresses or constant values. |
| `assembler/first_pass.*` | Symbol collection, instruction-size calculation, and 238-byte program-capacity validation. |
| `assembler/byte_literal.*` | Strict conversion of decimal and hexadecimal text to `uint8_t`. |
| `assembler/byte_operand.*` | Resolution of a literal or symbol into one byte. |
| `assembler/instruction_parser.*` | Separation and structural validation of instruction mnemonics and operands. |
| `assembler/instruction_encoder.*` | Semantic validation and conversion of parsed instructions into opcode and operand bytes. |
| `assembler/second_pass.*` | Label skipping, instruction encoding, bounded byte accumulation, and source-located diagnostics. |
| `assembler/binary_writer.*` | Exact raw-byte output with open, write, and close validation. |
| `assembler/main.c` | Argument handling, two-pass orchestration, pass-consistency checks, and binary-output coordination. |
| `tests/` | Independent C unit and integration tests plus a Bash process test of the complete simulator executable. |

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
make run INPUT_VALUE=0xA5
```

Runs the same demonstration with host input `0xA5`. The input is observable even though the built-in program does not consume it.

```bash
make run-bin
```

Builds the simulator and assembler, translates `programs/demo.asm` into `build/demo.bin`, loads that binary, and executes it.

```bash
make trace
```

Builds and executes the built-in demonstration while printing one trace entry after every attempted instruction.

```bash
make trace INPUT_VALUE=165
```

Tracing and normal execution accept the same input in decimal or hexadecimal form.

```bash
make trace-bin
```

Assembles `programs/demo.asm`, loads `build/demo.bin`, and executes it with the same trace format.

An already existing compatible binary can be executed directly:

```bash
./build/vm8 run path/to/program.bin
```

Supply input to that binary by placing the option last:

```bash
./build/vm8 run path/to/program.bin --input 0xA5
```

Trace an already existing compatible binary directly:

```bash
./build/vm8 trace path/to/program.bin
```

```bash
./build/vm8 trace path/to/program.bin --input 165
```

```bash
make help
```

Displays simulator commands and the instruction reference.

```bash
make test
```

Assembles the demonstration and runs all automated unit, integration, and process-level tests. Dedicated tests verify every current opcode-to-mnemonic mapping, rejection of an unknown opcode, observer delivery, exact trace formatting, memory-map boundaries, port direction, port reset, and strict CLI input parsing. The assembled-program test reads `build/demo.bin`, loads it into CPU memory, executes it, and verifies the expected registers, flags, program counter, cycle count, and stored data. `tests/test_vm8_process.sh` then launches the real executable and checks normal execution, both trace modes, real input-to-output transfer, invalid input, and binary-file failures.

```bash
make assembler
```

Builds the standalone `build/vm8asm` executable.

```bash
make assemble
```

Builds `vm8asm` when necessary, runs both passes on `programs/demo.asm`, and writes the resulting 19 raw bytes to `build/demo.bin`.

The same assembler can be invoked directly with explicit paths:

```bash
./build/vm8asm programs/demo.asm build/demo.bin
```

Check the exact output size:

```bash
wc -c build/demo.bin
```

Expected output:

```text
19 build/demo.bin
```

`wc -c` counts bytes rather than lines or words. This confirms the binary contains the 19 bytes calculated by both assembler passes.

Display every raw byte as hexadecimal:

```bash
od -An -tx1 -v build/demo.bin
```

Expected output:

```text
 10 2a 11 2a 21 30 09 10 ff 40 12 41 80 10 00 40
 80 01 5a
```

The `od` options mean:

- `-An`: omit the address column;
- `-tx1`: format each one-byte unit in hexadecimal;
- `-v`: display all data instead of abbreviating repeated lines.

This reads the binary as bytes. Running `cat build/demo.bin` is not useful because many machine-code byte values are not printable characters.

Assemble the demonstration and run both inspections in one step:

```bash
make inspect
```

## Key ideas to retain

- Eight-bit values are fetched and processed one byte at a time; an instruction may contain multiple bytes.
- The opcode tells the CPU how many additional bytes to fetch and how to interpret them.
- Opcode definitions belong to the instruction-set module rather than to the complete CPU interface.
- The metadata lookup accepts a raw byte and returns null when that byte is not a supported opcode.
- Logical operations process corresponding bits independently, while shifts preserve the discarded bit in the carry flag.
- `CMP` updates zero and borrow without changing its operands; conditional jumps inspect flags without changing them.
- Labels and mnemonics belong to the assembler, not to the CPU.
- A label consumes no program memory; it names the current byte address.
- `.EQU` gives an 8-bit literal a reusable symbolic name and emits no byte.
- `.BYTE` emits one raw byte and can place data in the same memory image as code.
- Labels and constants share one case-sensitive symbol namespace.
- The first pass discovers addresses, and the second pass replaces symbolic references with numeric bytes.
- The binary writer stores the generated values as raw bytes, not hexadecimal text.
- The binary reader uses a caller-provided capacity and checks one extra byte to reject oversized input safely.
- Memory-mapped I/O reuses `LDA` and `STA`; the decoded address selects ordinary memory, the input port, or the output latch.
- The host controls input `0xEE`, VM8 software controls output `0xEF`, and reset clears both.
- The CLI accepts a final `--input` value from 0 through 255 in decimal or `0x` hexadecimal form.
- `wc -c` verifies the byte count, while `od -An -tx1 -v` exposes the exact byte values.
- Built-in and file-loaded programs use the same CPU loading and execution functions.
- An observer receives the instruction address and opcode from before a step together with the CPU state from after that step.
- The trace is presentation layered on top of CPU execution; the CPU core does not print anything itself.
- The CPU ultimately executes only a byte sequence, regardless of where those bytes originated.
- Unit tests validate functions in isolation, while the Bash process test validates the compiled program through its public command-line interface.
- `PC` measures byte addresses, while the simplified cycle counter measures attempted instructions.
- Unified memory permits both code and data access, so stores must use addresses carefully.
- The loaded program is limited to `0x00` through `0xED`; `0xEE` and `0xEF` provide I/O, while the stack reserves `0xF0` through `0xFF` and grows downward.
- `SP = 0x00` is an empty-stack sentinel; `PUSH` and `POP` move bytes in last-in, first-out order and report boundary errors explicitly.
- `CALL` pushes the already-advanced `PC` as an 8-bit return address; `RET` pops that byte back into `PC`.
- Data and return addresses share the same stack, so balanced operations and available stack depth are software responsibilities.
