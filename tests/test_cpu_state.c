#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"
#include "cpu_state.h"
#include "test_stream.h"

enum
{
  STATE_BUFFER_CAPACITY = 512,
  STATE_REGISTER_A = 0xA5,
  STATE_REGISTER_B = 0x5A,
  STATE_PROGRAM_COUNTER = 0x80,
  STATE_STACK_POINTER = CPU_STACK_HIGH_ADDRESS - 1,
  STATE_INPUT_PORT = 0x3C,
  STATE_OUTPUT_PORT = 0xC3
};

static void test_prints_cpu_state(void)
{
  static const char expected_output[] =
    "Register A: 0xA5\n"
    "Register B: 0x5A\n"
    "Stack pointer: 0xFE\n"
    "Input port: 0x3C\n"
    "Output port: 0xC3\n"
    "Zero flag: set\n"
    "Carry flag: clear\n"
    "Program counter: 128\n"
    "Cycle count: 42\n";

  const uint64_t state_cycle_count = UINT64_C(42);

  Cpu cpu =
  {
    .register_a = STATE_REGISTER_A,
    .register_b = STATE_REGISTER_B,
    .program_counter = STATE_PROGRAM_COUNTER,
    .stack_pointer = STATE_STACK_POINTER,
    .input_port = 0,
    .output_port = 0,
    .zero_flag = true,
    .carry_flag = false,
    .halted = false,
    .cycle_count = state_cycle_count,
    .memory = {0}
  };

  cpu_set_input_port(&cpu, STATE_INPUT_PORT);

  cpu_write_memory(
    &cpu,
    CPU_OUTPUT_PORT_ADDRESS,
    STATE_OUTPUT_PORT
  );

  FILE *const output = test_tmpfile();

  assert(output != NULL);

  cpu_state_print(output, &cpu);

  const int flush_result = fflush(output);

  assert(flush_result == 0);

  rewind(output);

  char actual_output[STATE_BUFFER_CAPACITY] = {0};

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
  test_prints_cpu_state();

  puts("All CPU-state tests passed.");

  return EXIT_SUCCESS;
}
