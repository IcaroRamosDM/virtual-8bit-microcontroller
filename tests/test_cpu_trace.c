#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"
#include "cpu_trace.h"

enum
{
  TRACE_BUFFER_CAPACITY = 1024,
  TRACE_INSTRUCTION_ADDRESS = 0x04,
  TRACE_INVALID_INSTRUCTION_ADDRESS = 0x06,
  TRACE_STACK_OVERFLOW_ADDRESS = 0x07,
  TRACE_STACK_UNDERFLOW_ADDRESS = 0x08,
  TRACE_NEXT_PROGRAM_COUNTER = 0x05,
  TRACE_REGISTER_A = 0x30,
  TRACE_REGISTER_B = 0x10,
  TRACE_STACK_POINTER = CPU_STACK_HIGH_ADDRESS - 1,
  TRACE_INPUT_PORT = 0xA5,
  TRACE_OUTPUT_PORT = 0x5A,
  TRACE_CYCLE_COUNT = 7
};

static void test_prints_trace_entry(void)
{
  static const char expected_output[] =
    "Execution trace:\n"
    "  ADDR=0x04 OP=0x20 MNEMONIC=ADD "
    "A=0x30 B=0x10 SP=0xFE "
    "IN=0xA5 OUT=0x5A "
    "Z=0 C=1 NEXT=0x05 "
    "CYCLES=7 RESULT=ok\n"
    "  ADDR=0x06 OP=0xFF MNEMONIC=UNKNOWN "
    "A=0x30 B=0x10 SP=0xFE "
    "IN=0xA5 OUT=0x5A "
    "Z=0 C=1 NEXT=0x05 "
    "CYCLES=7 RESULT=invalid-opcode\n"
    "  ADDR=0x07 OP=0x50 MNEMONIC=PUSH "
    "A=0x30 B=0x10 SP=0xFE "
    "IN=0xA5 OUT=0x5A "
    "Z=0 C=1 NEXT=0x05 "
    "CYCLES=7 RESULT=stack-overflow\n"
    "  ADDR=0x08 OP=0x51 MNEMONIC=POP "
    "A=0x30 B=0x10 SP=0xFE "
    "IN=0xA5 OUT=0x5A "
    "Z=0 C=1 NEXT=0x05 "
    "CYCLES=7 RESULT=stack-underflow\n";

  Cpu cpu =
  {
    .register_a = TRACE_REGISTER_A,
    .register_b = TRACE_REGISTER_B,
    .program_counter = TRACE_NEXT_PROGRAM_COUNTER,
    .stack_pointer = TRACE_STACK_POINTER,
    .input_port = TRACE_INPUT_PORT,
    .output_port = TRACE_OUTPUT_PORT,
    .zero_flag = false,
    .carry_flag = true,
    .halted = false,
    .cycle_count = TRACE_CYCLE_COUNT,
    .memory = {0}
  };

  FILE *const output = tmpfile();

  assert(output != NULL);

  cpu_trace_print_header(output);

  cpu_trace_observer(
    TRACE_INSTRUCTION_ADDRESS,
    OPCODE_ADD_A_B,
    &cpu,
    CPU_STEP_OK,
    output
  );

  cpu_trace_observer(
    TRACE_INVALID_INSTRUCTION_ADDRESS,
    UINT8_MAX,
    &cpu,
    CPU_STEP_INVALID_OPCODE,
    output
  );

  cpu_trace_observer(
    TRACE_STACK_OVERFLOW_ADDRESS,
    OPCODE_PUSH_A,
    &cpu,
    CPU_STEP_STACK_OVERFLOW,
    output
  );

  cpu_trace_observer(
    TRACE_STACK_UNDERFLOW_ADDRESS,
    OPCODE_POP_A,
    &cpu,
    CPU_STEP_STACK_UNDERFLOW,
    output
  );

  const int flush_result = fflush(output);

  assert(flush_result == 0);

  rewind(output);

  char actual_output[TRACE_BUFFER_CAPACITY] = {0};

  const size_t read_byte_count = fread(
    actual_output,
    sizeof actual_output[0],
    sizeof actual_output - 1,
    output
  );

  const size_t expected_byte_count =
    strlen(expected_output);

  assert(read_byte_count == expected_byte_count);
  assert(strcmp(actual_output, expected_output) == 0);

  const int close_result = fclose(output);

  assert(close_result == 0);
}

int main(void)
{
  test_prints_trace_entry();

  puts("All CPU-trace tests passed.");

  return EXIT_SUCCESS;
}
