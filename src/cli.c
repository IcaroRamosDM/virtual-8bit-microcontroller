#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "cli.h"
#include "cpu.h"
#include "instruction_set.h"
#include "byte_value.h"

enum
{
  CLI_ARGUMENT_COUNT_WITHOUT_COMMAND = 1,
  CLI_ARGUMENT_COUNT_WITH_COMMAND = 2,
  CLI_ARGUMENT_COUNT_WITH_BINARY_PATH = 3,
  CLI_ARGUMENT_COUNT_WITH_INPUT = 4,
  CLI_ARGUMENT_COUNT_WITH_BINARY_AND_INPUT = 5,
  CLI_COMMAND_ARGUMENT_INDEX = 1,
  CLI_FIRST_VALUE_ARGUMENT_INDEX = 2,
  CLI_SECOND_VALUE_ARGUMENT_INDEX = 3,
  CLI_THIRD_VALUE_ARGUMENT_INDEX = 4
};

static const char HELP_COMMAND[] = "help";
static const char LONG_HELP_COMMAND[] = "--help";
static const char RUN_COMMAND[] = "run";
static const char TRACE_COMMAND[] = "trace";
static const char MONITOR_COMMAND[] = "monitor";
static const char INPUT_OPTION[] = "--input";

static CliOptions cli_make_options(
    CliCommand command,
    const char *binary_path,
    bool trace_enabled,
    uint8_t input_port_value
)
{
  return (CliOptions){
    .command = command,
    .binary_path = binary_path,
    .trace_enabled = trace_enabled,
    .input_port_value = input_port_value
  };
}

static CliOptions cli_invalid_options(void)
{
  return cli_make_options(
    CLI_COMMAND_INVALID,
    NULL,
    false,
    0
  );
}

static CliOptions cli_run_options(
    const char *binary_path,
    bool trace_enabled,
    uint8_t input_port_value
)
{
  const CliCommand command =
    (binary_path == NULL)
      ? CLI_COMMAND_RUN_DEMO
      : CLI_COMMAND_RUN_BINARY;

  return cli_make_options(
    command,
    binary_path,
    trace_enabled,
    input_port_value
  );
}

static CliOptions cli_monitor_options(
    const char *binary_path
)
{
  const CliCommand command =
    (binary_path == NULL)
      ? CLI_COMMAND_MONITOR_DEMO
      : CLI_COMMAND_MONITOR_BINARY;

  return cli_make_options(
    command,
    binary_path,
    false,
    0
  );
}

static bool cli_parse_run_command(
    const char *command,
    bool *trace_enabled
)
{
  if (strcmp(command, RUN_COMMAND) == 0)
  {
    *trace_enabled = false;
    return true;
  }

  if (strcmp(command, TRACE_COMMAND) == 0)
  {
    *trace_enabled = true;
    return true;
  }

  return false;
}

CliOptions cli_parse_arguments(
    int argument_count,
    char *arguments[]
)
{
  if (argument_count == CLI_ARGUMENT_COUNT_WITHOUT_COMMAND)
  {
    return cli_run_options(NULL, false, 0);
  }

  if (argument_count < CLI_ARGUMENT_COUNT_WITH_COMMAND)
  {
    return cli_invalid_options();
  }

  const char *const command =
    arguments[CLI_COMMAND_ARGUMENT_INDEX];

  if (
    (argument_count == CLI_ARGUMENT_COUNT_WITH_COMMAND) &&
    ((strcmp(command, HELP_COMMAND) == 0) ||
     (strcmp(command, LONG_HELP_COMMAND) == 0))
  )
  {
    return cli_make_options(
      CLI_COMMAND_HELP,
      NULL,
      false,
      0
    );
  }

  if (strcmp(command, MONITOR_COMMAND) == 0)
  {
    if (argument_count == CLI_ARGUMENT_COUNT_WITH_COMMAND)
    {
      return cli_monitor_options(NULL);
    }

    if (
      argument_count ==
      CLI_ARGUMENT_COUNT_WITH_BINARY_PATH
    )
    {
      const char *const binary_path =
        arguments[CLI_FIRST_VALUE_ARGUMENT_INDEX];

      if (strcmp(binary_path, INPUT_OPTION) == 0)
      {
        return cli_invalid_options();
      }

      return cli_monitor_options(binary_path);
    }

    return cli_invalid_options();
  }

  bool trace_enabled = false;

  if (!cli_parse_run_command(command, &trace_enabled))
  {
    return cli_invalid_options();
  }

  if (argument_count == CLI_ARGUMENT_COUNT_WITH_COMMAND)
  {
    return cli_run_options(
      NULL,
      trace_enabled,
      0
    );
  }

  if (argument_count == CLI_ARGUMENT_COUNT_WITH_BINARY_PATH)
  {
    const char *const binary_path =
      arguments[CLI_FIRST_VALUE_ARGUMENT_INDEX];

    if (strcmp(binary_path, INPUT_OPTION) == 0)
    {
      return cli_invalid_options();
    }

    return cli_run_options(
      binary_path,
      trace_enabled,
      0
    );
  }

  if (argument_count == CLI_ARGUMENT_COUNT_WITH_INPUT)
  {
    const char *const input_option =
      arguments[CLI_FIRST_VALUE_ARGUMENT_INDEX];

    const char *const input_text =
      arguments[CLI_SECOND_VALUE_ARGUMENT_INDEX];

    uint8_t input_port_value = 0;

    if ((strcmp(input_option, INPUT_OPTION) != 0) ||
        !byte_value_parse(
          input_text,
          &input_port_value
        ))
    {
      return cli_invalid_options();
    }

    return cli_run_options(
      NULL,
      trace_enabled,
      input_port_value
    );
  }

  if (
    argument_count ==
    CLI_ARGUMENT_COUNT_WITH_BINARY_AND_INPUT
  )
  {
    const char *const binary_path =
      arguments[CLI_FIRST_VALUE_ARGUMENT_INDEX];

    const char *const input_option =
      arguments[CLI_SECOND_VALUE_ARGUMENT_INDEX];

    const char *const input_text =
      arguments[CLI_THIRD_VALUE_ARGUMENT_INDEX];

    uint8_t input_port_value = 0;

    if ((strcmp(binary_path, INPUT_OPTION) == 0) ||
        (strcmp(input_option, INPUT_OPTION) != 0) ||
        !byte_value_parse(
          input_text,
          &input_port_value
        ))
    {
      return cli_invalid_options();
    }

    return cli_run_options(
      binary_path,
      trace_enabled,
      input_port_value
    );
  }

  return cli_invalid_options();
}

void cli_print_help(void)
{
  puts("VM8 virtual 8-bit microcontroller simulator");
  puts("");

  puts("Commands:");
  puts("  make run        Run the built-in demonstration.");
  puts("  make run-bin    Assemble and run programs/demo.asm.");
  puts("  make run-popcount  Assemble and run the popcount firmware.");
  puts("  make trace      Run the built-in demo with a trace.");
  puts("  make trace-bin  Assemble and trace programs/demo.asm.");
  puts("  make trace-popcount  Assemble and trace the popcount firmware.");
  puts("  make monitor    Open the monitor with the built-in demo.");
  puts("  make monitor-bin  Assemble and monitor programs/demo.asm.");
  puts("  make monitor-popcount  Assemble and monitor the popcount firmware.");
  puts("  make test       Build and run the test suite.");
  puts("  make assembler  Build the assembler executable.");
  puts("  make assemble   Run the assembler on programs/demo.asm.");
  puts("  make assemble-popcount  Assemble programs/popcount.asm.");
  puts("  make inspect    Assemble and inspect the demo binary.");
  puts("  make inspect-popcount  Assemble and inspect the popcount binary.");
  puts("  make help       Build and display this help.");
  puts("  make clean      Remove generated build files.");
  puts("");

  puts("Direct executable commands:");
  puts("  ./build/vm8");
  puts("  ./build/vm8 run [--input <byte>]");
  puts("  ./build/vm8 run <program.bin> [--input <byte>]");
  puts("  ./build/vm8 trace [--input <byte>]");
  puts("  ./build/vm8 trace <program.bin> [--input <byte>]");
  puts("  ./build/vm8 help");
  puts("  ./build/vm8 --help");
  puts("  ./build/vm8 monitor");
  puts("  ./build/vm8 monitor <program.bin>");
  puts(
    "  ./build/vm8asm <input.asm> <output.bin>  "
    "Assemble source into raw binary."
  );
  puts("");

  puts("Virtual input:");
  puts(
    "  <byte> accepts decimal or 0x-prefixed hexadecimal "
    "from 0 through 255."
  );
  puts("  The default input value is 0x00.");
  puts("  When present, --input <byte> must be the final option.");
  puts("  Make example: make run INPUT_VALUE=0xA5");
  puts("  Direct example: ./build/vm8 run firmware.bin --input 165");
  puts("");

  puts("Popcount firmware:");
  puts("  Reads the byte at input port 0xEE.");
  puts("  Counts its set bits and writes the result from 0 through 8");
  puts("  to output port 0xEF.");
  puts("  Run example: make run-popcount INPUT_VALUE=0xA5");
  puts("  Trace example: make trace-popcount INPUT_VALUE=0xA5");
  puts("  Monitor example: make monitor-popcount");
  puts("");

  puts("Interactive monitor:");
  puts("  Start the built-in demo with 'make monitor'.");
  puts("  Assemble and open programs/demo.asm with 'make monitor-bin'.");
  puts("  Assemble and open the popcount firmware with");
  puts("  'make monitor-popcount'.");
  puts(
    "  Commands: help, registers, step, run, reset, input, memory, "
    "load,"
  );
  puts("            trace, breakpoint, and quit.");
  puts(
    "  Breakpoints stop before the marked instruction; "
    "step executes it."
  );
  puts("  Run 'help' inside the monitor for complete command syntax.");
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
    "  AND A, B        Opcode: 0x%02X | Size: 1 byte\n",
    (unsigned int)OPCODE_AND_A_B
  );
  puts("    Effect: A <- A & B; Z <- (A == 0); C <- 0.");
  puts("    Explanation: Performs a bitwise AND between A and B.");
  puts("    The result is stored in A; B is unchanged.");
  puts("    Flags: Z reports a zero result; C is cleared.");
  puts("    Assembly example: AND A, B");
  printf(
    "    Encoding: 0x%02X\n",
    (unsigned int)OPCODE_AND_A_B
  );
  puts("");

  printf(
    "  OR A, B         Opcode: 0x%02X | Size: 1 byte\n",
    (unsigned int)OPCODE_OR_A_B
  );
  puts("    Effect: A <- A | B; Z <- (A == 0); C <- 0.");
  puts("    Explanation: Performs a bitwise OR between A and B.");
  puts("    The result is stored in A; B is unchanged.");
  puts("    Flags: Z reports a zero result; C is cleared.");
  puts("    Assembly example: OR A, B");
  printf(
    "    Encoding: 0x%02X\n",
    (unsigned int)OPCODE_OR_A_B
  );
  puts("");

  printf(
    "  XOR A, B        Opcode: 0x%02X | Size: 1 byte\n",
    (unsigned int)OPCODE_XOR_A_B
  );
  puts("    Effect: A <- A ^ B; Z <- (A == 0); C <- 0.");
  puts("    Explanation: Performs a bitwise XOR between A and B.");
  puts("    The result is stored in A; B is unchanged.");
  puts("    Flags: Z reports a zero result; C is cleared.");
  puts("    Assembly example: XOR A, B");
  printf(
    "    Encoding: 0x%02X\n",
    (unsigned int)OPCODE_XOR_A_B
  );
  puts("");

  printf(
    "  NOT A           Opcode: 0x%02X | Size: 1 byte\n",
    (unsigned int)OPCODE_NOT_A
  );
  puts("    Effect: A <- ~A; Z <- (A == 0); C <- 0.");
  puts("    Explanation: Inverts every bit in register A.");
  puts("    Register B is unchanged.");
  puts("    Flags: Z reports a zero result; C is cleared.");
  puts("    Assembly example: NOT A");
  printf(
    "    Encoding: 0x%02X\n",
    (unsigned int)OPCODE_NOT_A
  );
  puts("");

  printf(
    "  SHL A           Opcode: 0x%02X | Size: 1 byte\n",
    (unsigned int)OPCODE_SHIFT_LEFT_A
  );
  puts(
    "    Effect: C <- old A[7]; "
    "A <- (A << 1) mod 256; Z <- (A == 0)."
  );
  puts("    Explanation: Shifts A left and inserts zero into bit 0.");
  puts("    The former bit 7 moves into C; B is unchanged.");
  puts("    Assembly example: SHL A");
  printf(
    "    Encoding: 0x%02X\n",
    (unsigned int)OPCODE_SHIFT_LEFT_A
  );
  puts("");

  printf(
    "  SHR A           Opcode: 0x%02X | Size: 1 byte\n",
    (unsigned int)OPCODE_SHIFT_RIGHT_A
  );
  puts("    Effect: C <- old A[0]; A <- A >> 1; Z <- (A == 0).");
  puts("    Explanation: Shifts A right and inserts zero into bit 7.");
  puts("    The former bit 0 moves into C; B is unchanged.");
  puts("    Assembly example: SHR A");
  printf(
    "    Encoding: 0x%02X\n",
    (unsigned int)OPCODE_SHIFT_RIGHT_A
  );
  puts("");

  printf(
    "  CMP A, B        Opcode: 0x%02X | Size: 1 byte\n",
    (unsigned int)OPCODE_COMPARE_A_B
  );
  puts("    Effect: Z <- (A == B); C <- (A < B); A and B unchanged.");
  puts("    Explanation: Compares unsigned A with B as if calculating A - B.");
  puts("    The subtraction result is discarded.");
  puts("    Flags: Z reports equality; C reports an unsigned borrow.");
  puts("    Assembly example: CMP A, B");
  printf(
    "    Encoding: 0x%02X\n",
    (unsigned int)OPCODE_COMPARE_A_B
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
    "  JNZ addr8       Opcode: 0x%02X | Size: 2 bytes\n",
    (unsigned int)OPCODE_JUMP_IF_NOT_ZERO
  );
  puts("    Effect: PC <- addr8 if Z == 0; otherwise execution continues.");
  puts("    Explanation: Jumps to an absolute address when Z is clear.");
  puts("    Registers and flags are unchanged.");
  puts("    Assembly example: JNZ 0x80");
  printf(
    "    Encoding example: 0x%02X 0x80\n",
    (unsigned int)OPCODE_JUMP_IF_NOT_ZERO
  );
  puts("");

  printf(
    "  JC addr8        Opcode: 0x%02X | Size: 2 bytes\n",
    (unsigned int)OPCODE_JUMP_IF_CARRY
  );
  puts("    Effect: PC <- addr8 if C == 1; otherwise execution continues.");
  puts("    Explanation: Jumps to an absolute address when C is set.");
  puts("    After CMP, this means that unsigned A is less than B.");
  puts("    Registers and flags are unchanged.");
  puts("    Assembly example: JC 0x80");
  printf(
    "    Encoding example: 0x%02X 0x80\n",
    (unsigned int)OPCODE_JUMP_IF_CARRY
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

  printf(
    "  PUSH A          Opcode: 0x%02X | Size: 1 byte\n",
    (unsigned int)OPCODE_PUSH_A
  );
  puts(
    "    Effect: SP <- 0xFF if empty, otherwise SP - 1; "
    "memory[SP] <- A."
  );
  puts("    Explanation: Pushes A onto the downward-growing stack.");
  puts("    Registers A and B and flags Z and C are unchanged.");
  puts("    Pushing past address 0xF0 causes stack overflow.");
  puts("    Assembly example: PUSH A");
  printf(
    "    Encoding: 0x%02X\n",
    (unsigned int)OPCODE_PUSH_A
  );
  puts("");

  printf(
    "  POP A           Opcode: 0x%02X | Size: 1 byte\n",
    (unsigned int)OPCODE_POP_A
  );
  puts(
    "    Effect: A <- memory[SP]; SP <- 0x00 if SP was 0xFF, "
    "otherwise SP + 1."
  );
  puts("    Explanation: Pops the newest stack byte into A.");
  puts("    Z <- (A == 0); C unchanged; B and memory unchanged.");
  puts("    Popping when SP is 0x00 causes stack underflow.");
  puts("    Assembly example: POP A");
  printf(
    "    Encoding: 0x%02X\n",
    (unsigned int)OPCODE_POP_A
  );
  puts("");

  printf(
    "  CALL addr8      Opcode: 0x%02X | Size: 2 bytes\n",
    (unsigned int)OPCODE_CALL
  );
  puts("    Effect: Push address after addr8; PC <- addr8.");
  puts(
    "    Explanation: Saves the return address, then jumps "
    "to an absolute address."
  );
  puts("    Registers A and B and flags Z and C are unchanged.");
  puts("    A full stack causes stack overflow.");
  puts("    Assembly example: CALL subroutine");
  printf(
    "    Encoding example: 0x%02X 0x80\n",
    (unsigned int)OPCODE_CALL
  );
  puts("");

  printf(
    "  RET             Opcode: 0x%02X | Size: 1 byte\n",
    (unsigned int)OPCODE_RETURN
  );
  puts("    Effect: PC <- newest stack byte.");
  puts(
    "    Explanation: Pops the saved return address "
    "and resumes execution there."
  );
  puts("    Registers A and B and flags Z and C are unchanged.");
  puts("    An empty stack causes stack underflow.");
  puts("    Assembly example: RET");
  printf(
    "    Encoding: 0x%02X\n",
    (unsigned int)OPCODE_RETURN
  );
  puts("");

  puts("Memory map:");
  printf(
    "  Program binaries may use 0x00 through 0x%02X "
    "(%u bytes).\n",
    (unsigned int)(CPU_PROGRAM_MEMORY_SIZE - 1),
    (unsigned int)CPU_PROGRAM_MEMORY_SIZE
  );
  printf(
    "  Input port: 0x%02X (read-only to VM8 software).\n",
    (unsigned int)CPU_INPUT_PORT_ADDRESS
  );
  printf(
    "  Output port: 0x%02X (readable and writable latch).\n",
    (unsigned int)CPU_OUTPUT_PORT_ADDRESS
  );
  printf(
    "  The %u-byte stack uses addresses 0x%02X through "
    "0x%02X.\n",
    (unsigned int)CPU_STACK_CAPACITY,
    (unsigned int)CPU_STACK_LOW_ADDRESS,
    (unsigned int)CPU_STACK_HIGH_ADDRESS
  );
  puts("  SP = 0x00 represents an empty stack.");
  puts("");

  puts("Assembler directives:");
  puts("  .EQU NAME, value  Define a named 8-bit constant; emits no bytes.");
  puts("    NAME is case-sensitive and shares a namespace with labels.");
  puts("    value must be a decimal or 0x hexadecimal literal from 0 to 255.");
  puts("    Example: .EQU DATA_ADDRESS, 0x80");
  puts("  .BYTE value       Emit exactly one raw byte.");
  puts("    value may be a byte literal, a named constant, or a label.");
  puts("    Examples: .BYTE 0xA5 and .BYTE DATA_ADDRESS");
  puts("");

  puts("Simulator workflow:");
  puts("  Run the built-in demonstration with: make run");
  puts("  Assemble and run programs/demo.asm with: make run-bin");
  puts("  Run the popcount firmware with: make run-popcount INPUT_VALUE=0xA5");
  puts("  Run another binary with: ./build/vm8 run <program.bin>");
  puts("  Trace the built-in demonstration with: make trace");
  puts("  Assemble and trace programs/demo.asm with make trace-bin");
  puts("  Trace the popcount firmware with: make trace-popcount INPUT_VALUE=0xA5");
  puts("");

  puts("Assembler workflow:");
  puts("  Write assembly source in programs/demo.asm.");
  puts("  Build the assembler with: make assembler");
  puts("  Generate build/demo.bin with: make assemble");
  puts("  Generate build/popcount.bin with: make assemble-popcount");
  puts("  Display its size and raw bytes with: make inspect");
  puts("  Inspect the popcount binary with: make inspect-popcount");
  puts("  Size only: wc -c build/demo.bin");
  puts("  Bytes only: od -An -tx1 -v build/demo.bin");
}
