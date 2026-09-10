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

static void cli_help_puts(FILE *stream, const char *text)
{
  (void)fputs(text, stream);
  (void)fputc('\n', stream);
}

void cli_write_help(FILE *stream)
{
  cli_help_puts(stream, "VM8 virtual 8-bit microcontroller simulator");
  cli_help_puts(stream, "");

  cli_help_puts(stream, "Commands:");
  cli_help_puts(stream, "  make run        Run the built-in demonstration.");
  cli_help_puts(stream, "  make run-bin    Assemble and run programs/demo.asm.");
  cli_help_puts(stream, "  make run-popcount  Assemble and run the popcount firmware.");
  cli_help_puts(stream, "  make trace      Run the built-in demo with a trace.");
  cli_help_puts(stream, "  make trace-bin  Assemble and trace programs/demo.asm.");
  cli_help_puts(stream, "  make trace-popcount  Assemble and trace the popcount firmware.");
  cli_help_puts(stream, "  make monitor    Open the monitor with the built-in demo.");
  cli_help_puts(stream, "  make monitor-bin  Assemble and monitor programs/demo.asm.");
  cli_help_puts(stream, "  make monitor-popcount  Assemble and monitor the popcount firmware.");
  cli_help_puts(stream, "  make test       Build and run the test suite.");
  cli_help_puts(stream, "  make assembler  Build the assembler executable.");
  cli_help_puts(stream, "  make assemble   Run the assembler on programs/demo.asm.");
  cli_help_puts(stream, "  make assemble-popcount  Assemble programs/popcount.asm.");
  cli_help_puts(stream, "  make inspect    Assemble and inspect the demo binary.");
  cli_help_puts(stream, "  make inspect-popcount  Assemble and inspect the popcount binary.");
  cli_help_puts(stream, "  make help       Build and display this help.");
  cli_help_puts(stream, "  make clean      Remove generated build files.");
  cli_help_puts(stream, "");

  cli_help_puts(stream, "Direct executable commands:");
  cli_help_puts(stream, "  ./build/vm8");
  cli_help_puts(stream, "  ./build/vm8 run [--input <byte>]");
  cli_help_puts(stream, "  ./build/vm8 run <program.bin> [--input <byte>]");
  cli_help_puts(stream, "  ./build/vm8 trace [--input <byte>]");
  cli_help_puts(stream, "  ./build/vm8 trace <program.bin> [--input <byte>]");
  cli_help_puts(stream, "  ./build/vm8 help");
  cli_help_puts(stream, "  ./build/vm8 --help");
  cli_help_puts(stream, "  ./build/vm8 monitor");
  cli_help_puts(stream, "  ./build/vm8 monitor <program.bin>");
  cli_help_puts(stream,
    "  ./build/vm8asm <input.asm> <output.bin>  "
    "Assemble source into raw binary."
  );
  cli_help_puts(stream, "");

  cli_help_puts(stream, "Virtual input:");
  cli_help_puts(stream,
    "  <byte> accepts decimal or 0x-prefixed hexadecimal "
    "from 0 through 255."
  );
  cli_help_puts(stream, "  The default input value is 0x00.");
  cli_help_puts(stream, "  When present, --input <byte> must be the final option.");
  cli_help_puts(stream, "  Make example: make run INPUT_VALUE=0xA5");
  cli_help_puts(stream, "  Direct example: ./build/vm8 run firmware.bin --input 165");
  cli_help_puts(stream, "");

  cli_help_puts(stream, "Popcount firmware:");
  cli_help_puts(stream, "  Reads the byte at input port 0xEE.");
  cli_help_puts(stream, "  Counts its set bits and writes the result from 0 through 8");
  cli_help_puts(stream, "  to output port 0xEF.");
  cli_help_puts(stream, "  Run example: make run-popcount INPUT_VALUE=0xA5");
  cli_help_puts(stream, "  Trace example: make trace-popcount INPUT_VALUE=0xA5");
  cli_help_puts(stream, "  Monitor example: make monitor-popcount");
  cli_help_puts(stream, "");

  cli_help_puts(stream, "Interactive monitor:");
  cli_help_puts(stream, "  Start the built-in demo with 'make monitor'.");
  cli_help_puts(stream, "  Assemble and open programs/demo.asm with 'make monitor-bin'.");
  cli_help_puts(stream, "  Assemble and open the popcount firmware with");
  cli_help_puts(stream, "  'make monitor-popcount'.");
  cli_help_puts(stream,
    "  Commands: help, registers, step, run, reset, input, memory, "
    "load,"
  );
  cli_help_puts(stream, "            trace, breakpoint, and quit.");
  cli_help_puts(stream,
    "  Breakpoints stop before the marked instruction; "
    "step executes it."
  );
  cli_help_puts(stream, "  Run 'help' inside the monitor for complete command syntax.");
  cli_help_puts(stream, "");

  cli_help_puts(stream, "Supported instructions:");

  fprintf(stream,
    "  NOP             Opcode: 0x%02X | Size: 1 byte\n",
    (unsigned int)OPCODE_NOP
  );
  cli_help_puts(stream, "    Effect: Performs no data operation.");

  fprintf(stream,
    "  HALT            Opcode: 0x%02X | Size: 1 byte\n",
    (unsigned int)OPCODE_HALT
  );
  cli_help_puts(stream, "    Effect: Stops program execution.");

  fprintf(stream,
    "  LDI A, imm8     Opcode: 0x%02X | Size: 2 bytes\n",
    (unsigned int)OPCODE_LOAD_IMMEDIATE_A
  );
  cli_help_puts(stream, "    Effect: A <- imm8; Z <- (A == 0); C unchanged.");
  cli_help_puts(stream, "    Explanation: Loads the next 8-bit value into register A.");
  cli_help_puts(stream, "    Flags: Z is set if the value is zero; C is unchanged.");
  cli_help_puts(stream, "    Assembly example: LDI A, 0xA5");
  fprintf(stream,
    "    Encoding example: 0x%02X 0xA5\n",
    (unsigned int)OPCODE_LOAD_IMMEDIATE_A
  );
  cli_help_puts(stream, "");


  fprintf(stream,
    "  LDI B, imm8     Opcode: 0x%02X | Size: 2 bytes\n",
    (unsigned int)OPCODE_LOAD_IMMEDIATE_B
  );
  cli_help_puts(stream, "    Effect: B <- imm8; Z <- (B == 0); C unchanged.");
  cli_help_puts(stream, "    Explanation: Loads the next 8-bit value into register B.");
  cli_help_puts(stream, "    Flags: Z is set if the value is zero; C is unchanged.");
  cli_help_puts(stream, "    Assembly example: LDI B, 0x5A");
  fprintf(stream,
    "    Encoding example: 0x%02X 0x5A\n",
    (unsigned int)OPCODE_LOAD_IMMEDIATE_B
  );
  cli_help_puts(stream, "");

  fprintf(stream,
    "  ADD A, B        Opcode: 0x%02X | Size: 1 byte\n",
    (unsigned int)OPCODE_ADD_A_B
  );
  cli_help_puts(stream, "    Effect: A <- (A + B) mod 256; Z <- (A == 0); C <- carry.");
  cli_help_puts(stream, "    Explanation: Adds register B to register A.");
  cli_help_puts(stream, "    The 8-bit result is stored in A; B is unchanged.");
  cli_help_puts(stream, "    Flags: Z reports a zero result; C reports unsigned overflow.");
  cli_help_puts(stream, "    Assembly example: ADD A, B");
  fprintf(stream,
    "    Encoding: 0x%02X\n",
    (unsigned int)OPCODE_ADD_A_B
  );
  cli_help_puts(stream, "");

  fprintf(stream,
    "  SUB A, B        Opcode: 0x%02X | Size: 1 byte\n",
    (unsigned int)OPCODE_SUB_A_B
  );
  cli_help_puts(stream, "    Effect: A <- (A - B) mod 256; Z <- (A == 0); C <- borrow.");
  cli_help_puts(stream, "    Explanation: Subtracts register B from register A.");
  cli_help_puts(stream, "    The 8-bit result is stored in A; B is unchanged.");
  cli_help_puts(stream,
    "    Flags: Z reports a zero result; "
    "C is set when the original A is less than B."
  );
  cli_help_puts(stream, "    Assembly example: SUB A, B");
  fprintf(stream,
    "    Encoding: 0x%02X\n",
    (unsigned int)OPCODE_SUB_A_B
  );
  cli_help_puts(stream, "");

  fprintf(stream,
    "  AND A, B        Opcode: 0x%02X | Size: 1 byte\n",
    (unsigned int)OPCODE_AND_A_B
  );
  cli_help_puts(stream, "    Effect: A <- A & B; Z <- (A == 0); C <- 0.");
  cli_help_puts(stream, "    Explanation: Performs a bitwise AND between A and B.");
  cli_help_puts(stream, "    The result is stored in A; B is unchanged.");
  cli_help_puts(stream, "    Flags: Z reports a zero result; C is cleared.");
  cli_help_puts(stream, "    Assembly example: AND A, B");
  fprintf(stream,
    "    Encoding: 0x%02X\n",
    (unsigned int)OPCODE_AND_A_B
  );
  cli_help_puts(stream, "");

  fprintf(stream,
    "  OR A, B         Opcode: 0x%02X | Size: 1 byte\n",
    (unsigned int)OPCODE_OR_A_B
  );
  cli_help_puts(stream, "    Effect: A <- A | B; Z <- (A == 0); C <- 0.");
  cli_help_puts(stream, "    Explanation: Performs a bitwise OR between A and B.");
  cli_help_puts(stream, "    The result is stored in A; B is unchanged.");
  cli_help_puts(stream, "    Flags: Z reports a zero result; C is cleared.");
  cli_help_puts(stream, "    Assembly example: OR A, B");
  fprintf(stream,
    "    Encoding: 0x%02X\n",
    (unsigned int)OPCODE_OR_A_B
  );
  cli_help_puts(stream, "");

  fprintf(stream,
    "  XOR A, B        Opcode: 0x%02X | Size: 1 byte\n",
    (unsigned int)OPCODE_XOR_A_B
  );
  cli_help_puts(stream, "    Effect: A <- A ^ B; Z <- (A == 0); C <- 0.");
  cli_help_puts(stream, "    Explanation: Performs a bitwise XOR between A and B.");
  cli_help_puts(stream, "    The result is stored in A; B is unchanged.");
  cli_help_puts(stream, "    Flags: Z reports a zero result; C is cleared.");
  cli_help_puts(stream, "    Assembly example: XOR A, B");
  fprintf(stream,
    "    Encoding: 0x%02X\n",
    (unsigned int)OPCODE_XOR_A_B
  );
  cli_help_puts(stream, "");

  fprintf(stream,
    "  NOT A           Opcode: 0x%02X | Size: 1 byte\n",
    (unsigned int)OPCODE_NOT_A
  );
  cli_help_puts(stream, "    Effect: A <- ~A; Z <- (A == 0); C <- 0.");
  cli_help_puts(stream, "    Explanation: Inverts every bit in register A.");
  cli_help_puts(stream, "    Register B is unchanged.");
  cli_help_puts(stream, "    Flags: Z reports a zero result; C is cleared.");
  cli_help_puts(stream, "    Assembly example: NOT A");
  fprintf(stream,
    "    Encoding: 0x%02X\n",
    (unsigned int)OPCODE_NOT_A
  );
  cli_help_puts(stream, "");

  fprintf(stream,
    "  SHL A           Opcode: 0x%02X | Size: 1 byte\n",
    (unsigned int)OPCODE_SHIFT_LEFT_A
  );
  cli_help_puts(stream,
    "    Effect: C <- old A[7]; "
    "A <- (A << 1) mod 256; Z <- (A == 0)."
  );
  cli_help_puts(stream, "    Explanation: Shifts A left and inserts zero into bit 0.");
  cli_help_puts(stream, "    The former bit 7 moves into C; B is unchanged.");
  cli_help_puts(stream, "    Assembly example: SHL A");
  fprintf(stream,
    "    Encoding: 0x%02X\n",
    (unsigned int)OPCODE_SHIFT_LEFT_A
  );
  cli_help_puts(stream, "");

  fprintf(stream,
    "  SHR A           Opcode: 0x%02X | Size: 1 byte\n",
    (unsigned int)OPCODE_SHIFT_RIGHT_A
  );
  cli_help_puts(stream, "    Effect: C <- old A[0]; A <- A >> 1; Z <- (A == 0).");
  cli_help_puts(stream, "    Explanation: Shifts A right and inserts zero into bit 7.");
  cli_help_puts(stream, "    The former bit 0 moves into C; B is unchanged.");
  cli_help_puts(stream, "    Assembly example: SHR A");
  fprintf(stream,
    "    Encoding: 0x%02X\n",
    (unsigned int)OPCODE_SHIFT_RIGHT_A
  );
  cli_help_puts(stream, "");

  fprintf(stream,
    "  CMP A, B        Opcode: 0x%02X | Size: 1 byte\n",
    (unsigned int)OPCODE_COMPARE_A_B
  );
  cli_help_puts(stream, "    Effect: Z <- (A == B); C <- (A < B); A and B unchanged.");
  cli_help_puts(stream, "    Explanation: Compares unsigned A with B as if calculating A - B.");
  cli_help_puts(stream, "    The subtraction result is discarded.");
  cli_help_puts(stream, "    Flags: Z reports equality; C reports an unsigned borrow.");
  cli_help_puts(stream, "    Assembly example: CMP A, B");
  fprintf(stream,
    "    Encoding: 0x%02X\n",
    (unsigned int)OPCODE_COMPARE_A_B
  );
  cli_help_puts(stream, "");

  fprintf(stream,
    "  JZ addr8        Opcode: 0x%02X | Size: 2 bytes\n",
    (unsigned int)OPCODE_JUMP_IF_ZERO
  );
  cli_help_puts(stream, "    Effect: PC <- addr8 if Z == 1; otherwise execution continues.");
  cli_help_puts(stream, "    Explanation: Jumps to an absolute 8-bit address when Z is set.");
  cli_help_puts(stream, "    Registers and flags are unchanged.");
  cli_help_puts(stream, "    Assembly example: JZ 0x80");
  fprintf(stream,
    "    Encoding example: 0x%02X 0x80\n",
    (unsigned int)OPCODE_JUMP_IF_ZERO
  );
  cli_help_puts(stream, "");

  fprintf(stream,
    "  JNZ addr8       Opcode: 0x%02X | Size: 2 bytes\n",
    (unsigned int)OPCODE_JUMP_IF_NOT_ZERO
  );
  cli_help_puts(stream, "    Effect: PC <- addr8 if Z == 0; otherwise execution continues.");
  cli_help_puts(stream, "    Explanation: Jumps to an absolute address when Z is clear.");
  cli_help_puts(stream, "    Registers and flags are unchanged.");
  cli_help_puts(stream, "    Assembly example: JNZ 0x80");
  fprintf(stream,
    "    Encoding example: 0x%02X 0x80\n",
    (unsigned int)OPCODE_JUMP_IF_NOT_ZERO
  );
  cli_help_puts(stream, "");

  fprintf(stream,
    "  JC addr8        Opcode: 0x%02X | Size: 2 bytes\n",
    (unsigned int)OPCODE_JUMP_IF_CARRY
  );
  cli_help_puts(stream, "    Effect: PC <- addr8 if C == 1; otherwise execution continues.");
  cli_help_puts(stream, "    Explanation: Jumps to an absolute address when C is set.");
  cli_help_puts(stream, "    After CMP, this means that unsigned A is less than B.");
  cli_help_puts(stream, "    Registers and flags are unchanged.");
  cli_help_puts(stream, "    Assembly example: JC 0x80");
  fprintf(stream,
    "    Encoding example: 0x%02X 0x80\n",
    (unsigned int)OPCODE_JUMP_IF_CARRY
  );
  cli_help_puts(stream, "");

  fprintf(stream,
    "  JMP addr8       Opcode: 0x%02X | Size: 2 bytes\n",
    (unsigned int)OPCODE_JUMP
  );
  cli_help_puts(stream, "    Effect: PC <- addr8.");
  cli_help_puts(stream,
    "    Explanation: Jumps unconditionally "
    "to an absolute 8-bit address."
  );
  cli_help_puts(stream, "    Registers and flags are unchanged.");
  cli_help_puts(stream, "    Assembly example: JMP 0x80");
  fprintf(stream,
    "    Encoding example: 0x%02X 0x80\n",
    (unsigned int)OPCODE_JUMP
  );
  cli_help_puts(stream, "");

  fprintf(stream,
    "  LDA addr8       Opcode: 0x%02X | Size: 2 bytes\n",
    (unsigned int)OPCODE_LOAD_A_FROM_MEMORY
  );
  cli_help_puts(stream, "    Effect: A <- memory[addr8]; Z <- (A == 0); C unchanged.");
  cli_help_puts(stream,
    "    Explanation: Loads register A "
    "from an absolute memory address."
  );
  cli_help_puts(stream, "    Register B and memory are unchanged.");
  cli_help_puts(stream, "    Assembly example: LDA 0x80");
  fprintf(stream,
    "    Encoding example: 0x%02X 0x80\n",
    (unsigned int)OPCODE_LOAD_A_FROM_MEMORY
  );
  cli_help_puts(stream, "");

  fprintf(stream,
    "  STA addr8       Opcode: 0x%02X | Size: 2 bytes\n",
    (unsigned int)OPCODE_STORE_A_TO_MEMORY
  );
  cli_help_puts(stream, "    Effect: memory[addr8] <- A; registers and flags unchanged.");
  cli_help_puts(stream,
    "    Explanation: Stores register A "
    "at an absolute memory address."
  );
  cli_help_puts(stream, "    Assembly example: STA 0x80");
  fprintf(stream,
    "    Encoding example: 0x%02X 0x80\n",
    (unsigned int)OPCODE_STORE_A_TO_MEMORY
  );
  cli_help_puts(stream, "");

  fprintf(stream,
    "  PUSH A          Opcode: 0x%02X | Size: 1 byte\n",
    (unsigned int)OPCODE_PUSH_A
  );
  cli_help_puts(stream,
    "    Effect: SP <- 0xFF if empty, otherwise SP - 1; "
    "memory[SP] <- A."
  );
  cli_help_puts(stream, "    Explanation: Pushes A onto the downward-growing stack.");
  cli_help_puts(stream, "    Registers A and B and flags Z and C are unchanged.");
  cli_help_puts(stream, "    Pushing past address 0xF0 causes stack overflow.");
  cli_help_puts(stream, "    Assembly example: PUSH A");
  fprintf(stream,
    "    Encoding: 0x%02X\n",
    (unsigned int)OPCODE_PUSH_A
  );
  cli_help_puts(stream, "");

  fprintf(stream,
    "  POP A           Opcode: 0x%02X | Size: 1 byte\n",
    (unsigned int)OPCODE_POP_A
  );
  cli_help_puts(stream,
    "    Effect: A <- memory[SP]; SP <- 0x00 if SP was 0xFF, "
    "otherwise SP + 1."
  );
  cli_help_puts(stream, "    Explanation: Pops the newest stack byte into A.");
  cli_help_puts(stream, "    Z <- (A == 0); C unchanged; B and memory unchanged.");
  cli_help_puts(stream, "    Popping when SP is 0x00 causes stack underflow.");
  cli_help_puts(stream, "    Assembly example: POP A");
  fprintf(stream,
    "    Encoding: 0x%02X\n",
    (unsigned int)OPCODE_POP_A
  );
  cli_help_puts(stream, "");

  fprintf(stream,
    "  CALL addr8      Opcode: 0x%02X | Size: 2 bytes\n",
    (unsigned int)OPCODE_CALL
  );
  cli_help_puts(stream, "    Effect: Push address after addr8; PC <- addr8.");
  cli_help_puts(stream,
    "    Explanation: Saves the return address, then jumps "
    "to an absolute address."
  );
  cli_help_puts(stream, "    Registers A and B and flags Z and C are unchanged.");
  cli_help_puts(stream, "    A full stack causes stack overflow.");
  cli_help_puts(stream, "    Assembly example: CALL subroutine");
  fprintf(stream,
    "    Encoding example: 0x%02X 0x80\n",
    (unsigned int)OPCODE_CALL
  );
  cli_help_puts(stream, "");

  fprintf(stream,
    "  RET             Opcode: 0x%02X | Size: 1 byte\n",
    (unsigned int)OPCODE_RETURN
  );
  cli_help_puts(stream, "    Effect: PC <- newest stack byte.");
  cli_help_puts(stream,
    "    Explanation: Pops the saved return address "
    "and resumes execution there."
  );
  cli_help_puts(stream, "    Registers A and B and flags Z and C are unchanged.");
  cli_help_puts(stream, "    An empty stack causes stack underflow.");
  cli_help_puts(stream, "    Assembly example: RET");
  fprintf(stream,
    "    Encoding: 0x%02X\n",
    (unsigned int)OPCODE_RETURN
  );
  cli_help_puts(stream, "");

  cli_help_puts(stream, "Memory map:");
  fprintf(stream,
    "  Program binaries may use 0x00 through 0x%02X "
    "(%u bytes).\n",
    (unsigned int)(CPU_PROGRAM_MEMORY_SIZE - 1),
    (unsigned int)CPU_PROGRAM_MEMORY_SIZE
  );
  fprintf(stream,
    "  Input port: 0x%02X (read-only to VM8 software).\n",
    (unsigned int)CPU_INPUT_PORT_ADDRESS
  );
  fprintf(stream,
    "  Output port: 0x%02X (readable and writable latch).\n",
    (unsigned int)CPU_OUTPUT_PORT_ADDRESS
  );
  fprintf(stream,
    "  The %u-byte stack uses addresses 0x%02X through "
    "0x%02X.\n",
    (unsigned int)CPU_STACK_CAPACITY,
    (unsigned int)CPU_STACK_LOW_ADDRESS,
    (unsigned int)CPU_STACK_HIGH_ADDRESS
  );
  cli_help_puts(stream, "  SP = 0x00 represents an empty stack.");
  cli_help_puts(stream, "");

  cli_help_puts(stream, "Assembler directives:");
  cli_help_puts(stream, "  .EQU NAME, value  Define a named 8-bit constant; emits no bytes.");
  cli_help_puts(stream, "    NAME is case-sensitive and shares a namespace with labels.");
  cli_help_puts(stream, "    value must be a decimal or 0x hexadecimal literal from 0 to 255.");
  cli_help_puts(stream, "    Example: .EQU DATA_ADDRESS, 0x80");
  cli_help_puts(stream, "  .BYTE value       Emit exactly one raw byte.");
  cli_help_puts(stream, "    value may be a byte literal, a named constant, or a label.");
  cli_help_puts(stream, "    Examples: .BYTE 0xA5 and .BYTE DATA_ADDRESS");
  cli_help_puts(stream, "");

  cli_help_puts(stream, "Simulator workflow:");
  cli_help_puts(stream, "  Run the built-in demonstration with: make run");
  cli_help_puts(stream, "  Assemble and run programs/demo.asm with: make run-bin");
  cli_help_puts(stream, "  Run the popcount firmware with: make run-popcount INPUT_VALUE=0xA5");
  cli_help_puts(stream, "  Run another binary with: ./build/vm8 run <program.bin>");
  cli_help_puts(stream, "  Trace the built-in demonstration with: make trace");
  cli_help_puts(stream, "  Assemble and trace programs/demo.asm with make trace-bin");
  cli_help_puts(stream, "  Trace the popcount firmware with: make trace-popcount INPUT_VALUE=0xA5");
  cli_help_puts(stream, "");

  cli_help_puts(stream, "Assembler workflow:");
  cli_help_puts(stream, "  Write assembly source in programs/demo.asm.");
  cli_help_puts(stream, "  Build the assembler with: make assembler");
  cli_help_puts(stream, "  Generate build/demo.bin with: make assemble");
  cli_help_puts(stream, "  Generate build/popcount.bin with: make assemble-popcount");
  cli_help_puts(stream, "  Display its size and raw bytes with: make inspect");
  cli_help_puts(stream, "  Inspect the popcount binary with: make inspect-popcount");
  cli_help_puts(stream, "  Size only: wc -c build/demo.bin");
  cli_help_puts(stream, "  Bytes only: od -An -tx1 -v build/demo.bin");
}

void cli_print_help(void)
{
  cli_write_help(stdout);
}
