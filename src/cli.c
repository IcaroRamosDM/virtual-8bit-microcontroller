#include <stdio.h>
#include <string.h>

#include "cli.h"
#include "cpu.h"

enum
{
  CLI_ARGUMENT_COUNT_WITHOUT_COMMAND = 1,
  CLI_ARGUMENT_COUNT_WITH_COMMAND = 2,
  CLI_COMMAND_ARGUMENT_INDEX = 1
};

CliCommand cli_parse_command(
    int argument_count,
    char *arguments[]
)
{
  if (argument_count == CLI_ARGUMENT_COUNT_WITHOUT_COMMAND)
  {
    return CLI_COMMAND_RUN;
  }

  if (argument_count != CLI_ARGUMENT_COUNT_WITH_COMMAND)
  {
    return CLI_COMMAND_INVALID;
  }

  const char *command = arguments[CLI_COMMAND_ARGUMENT_INDEX];

  if ((strcmp(command, "help") == 0) ||
      (strcmp(command, "--help") == 0))
  {
    return CLI_COMMAND_HELP;
  }

  return CLI_COMMAND_INVALID;
}

void cli_print_help(void)
{
  puts("VM8 virtual 8-bit microcontroller simulator");
  puts("");

  puts("Commands:");
  puts("  make run        Build and run the demonstration program.");
  puts("  make test       Build and run the test suite.");
  puts("  make assembler  Build the assembler executable.");
  puts("  make assemble   Run the assembler on programs/demo.asm.");
  puts("  make inspect    Assemble and inspect the demo binary.");
  puts("  make help       Build and display this help.");
  puts("  make clean      Remove generated build files.");
  puts("");

  puts("Direct executable commands:");
  puts("  ./build/vm8          Run the demonstration program.");
  puts("  ./build/vm8 help     Display this help.");
  puts("  ./build/vm8 --help   Display this help.");
  puts(
    "  ./build/vm8asm <input.asm> <output.bin>  "
    "Assemble source into raw binary."
  );
  puts("");

  puts("Supported instructions:");

  printf(
    "  NOP             Opcode: 0x%02X | Size: 1 byte\n",
    (unsigned int)OPCODE_NOP
  );
  puts("    Effect: Performs no data operation.");

  printf(
    "  HALT            Opcode: 0x%02X | Size: 1 byte\n",
    (unsigned int)OPCODE_HALT
  );
  puts("    Effect: Stops program execution.");

  printf(
    "  LDI A, imm8     Opcode: 0x%02X | Size: 2 bytes\n",
    (unsigned int)OPCODE_LOAD_IMMEDIATE_A
  );
  puts("    Effect: A <- imm8; Z <- (A == 0); C unchanged.");
  puts("    Explanation: Loads the next 8-bit value into register A.");
  puts("    Flags: Z is set if the value is zero; C is unchanged.");
  puts("    Assembly example: LDI A, 0xA5");
  printf(
    "    Encoding example: 0x%02X 0xA5\n",
    (unsigned int)OPCODE_LOAD_IMMEDIATE_A
  );
  puts("");


  printf(
    "  LDI B, imm8     Opcode: 0x%02X | Size: 2 bytes\n",
    (unsigned int)OPCODE_LOAD_IMMEDIATE_B
  );
  puts("    Effect: B <- imm8; Z <- (B == 0); C unchanged.");
  puts("    Explanation: Loads the next 8-bit value into register B.");
  puts("    Flags: Z is set if the value is zero; C is unchanged.");
  puts("    Assembly example: LDI B, 0x5A");
  printf(
    "    Encoding example: 0x%02X 0x5A\n",
    (unsigned int)OPCODE_LOAD_IMMEDIATE_B
  );
  puts("");

  printf(
    "  ADD A, B        Opcode: 0x%02X | Size: 1 byte\n",
    (unsigned int)OPCODE_ADD_A_B
  );
  puts("    Effect: A <- (A + B) mod 256; Z <- (A == 0); C <- carry.");
  puts("    Explanation: Adds register B to register A.");
  puts("    The 8-bit result is stored in A; B is unchanged.");
  puts("    Flags: Z reports a zero result; C reports unsigned overflow.");
  puts("    Assembly example: ADD A, B");
  printf(
    "    Encoding: 0x%02X\n",
    (unsigned int)OPCODE_ADD_A_B
  );
  puts("");

  printf(
    "  SUB A, B        Opcode: 0x%02X | Size: 1 byte\n",
    (unsigned int)OPCODE_SUB_A_B
  );
  puts("    Effect: A <- (A - B) mod 256; Z <- (A == 0); C <- borrow.");
  puts("    Explanation: Subtracts register B from register A.");
  puts("    The 8-bit result is stored in A; B is unchanged.");
  puts(
    "    Flags: Z reports a zero result; "
    "C is set when the original A is less than B."
  );
  puts("    Assembly example: SUB A, B");
  printf(
    "    Encoding: 0x%02X\n",
    (unsigned int)OPCODE_SUB_A_B
  );
  puts("");

  printf(
    "  JZ addr8        Opcode: 0x%02X | Size: 2 bytes\n",
    (unsigned int)OPCODE_JUMP_IF_ZERO
  );
  puts("    Effect: PC <- addr8 if Z == 1; otherwise execution continues.");
  puts("    Explanation: Jumps to an absolute 8-bit address when Z is set.");
  puts("    Registers and flags are unchanged.");
  puts("    Assembly example: JZ 0x80");
  printf(
    "    Encoding example: 0x%02X 0x80\n",
    (unsigned int)OPCODE_JUMP_IF_ZERO
  );
  puts("");

  printf(
    "  JMP addr8       Opcode: 0x%02X | Size: 2 bytes\n",
    (unsigned int)OPCODE_JUMP
  );
  puts("    Effect: PC <- addr8.");
  puts(
    "    Explanation: Jumps unconditionally "
    "to an absolute 8-bit address."
  );
  puts("    Registers and flags are unchanged.");
  puts("    Assembly example: JMP 0x80");
  printf(
    "    Encoding example: 0x%02X 0x80\n",
    (unsigned int)OPCODE_JUMP
  );
  puts("");

  printf(
    "  LDA addr8       Opcode: 0x%02X | Size: 2 bytes\n",
    (unsigned int)OPCODE_LOAD_A_FROM_MEMORY
  );
  puts("    Effect: A <- memory[addr8]; Z <- (A == 0); C unchanged.");
  puts(
    "    Explanation: Loads register A "
    "from an absolute memory address."
  );
  puts("    Register B and memory are unchanged.");
  puts("    Assembly example: LDA 0x80");
  printf(
    "    Encoding example: 0x%02X 0x80\n",
    (unsigned int)OPCODE_LOAD_A_FROM_MEMORY
  );
  puts("");

  printf(
    "  STA addr8       Opcode: 0x%02X | Size: 2 bytes\n",
    (unsigned int)OPCODE_STORE_A_TO_MEMORY
  );
  puts("    Effect: memory[addr8] <- A; registers and flags unchanged.");
  puts(
    "    Explanation: Stores register A "
    "at an absolute memory address."
  );
  puts("    Assembly example: STA 0x80");
  printf(
    "    Encoding example: 0x%02X 0x80\n",
    (unsigned int)OPCODE_STORE_A_TO_MEMORY
  );
  puts("");
  puts("Current simulator workflow:");
  puts("  The demonstration bytecode is defined in src/program.c.");
  puts("  Build and execute it with: make run");
  puts("");

  puts("Current assembler workflow:");
  puts("  Write assembly source in programs/demo.asm.");
  puts("  Build the assembler with: make assembler");
  puts("  Generate build/demo.bin with: make assemble");
  puts("  Display its size and raw bytes with: make inspect");
  puts("  Size only: wc -c build/demo.bin");
  puts("  Bytes only: od -An -tx1 -v build/demo.bin");
}
