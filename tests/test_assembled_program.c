#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "binary_reader.h"
#include "cpu.h"

enum
{
  INITIAL_DATA_ADDRESS = 0x12,
  EXPECTED_PROGRAM_COUNTER = INITIAL_DATA_ADDRESS,
  EXPECTED_PROGRAM_SIZE = INITIAL_DATA_ADDRESS + 1
};

static const char ASSEMBLED_PROGRAM_PATH[] =
  "build/demo.bin";

static void test_executes_assembled_program(void)
{
  const uint8_t expected_register_a = UINT8_C(0x5A);
  const uint8_t expected_register_b = UINT8_C(0x2A);
  const uint8_t expected_program_counter =
    (uint8_t)EXPECTED_PROGRAM_COUNTER;
  const uint8_t data_address = UINT8_C(0x80);
  const uint8_t initial_data_address =
    (uint8_t)INITIAL_DATA_ADDRESS;
  const uint64_t expected_cycle_count = UINT64_C(9);
  const uint64_t instruction_limit = CPU_MEMORY_SIZE;

  uint8_t program_bytes[CPU_PROGRAM_MEMORY_SIZE] = {0};
  size_t program_size = 0;

  const bool read_succeeded = binary_reader_read(
    ASSEMBLED_PROGRAM_PATH,
    program_bytes,
    sizeof program_bytes,
    &program_size
  );

  assert(read_succeeded);
  assert(program_size == EXPECTED_PROGRAM_SIZE);

  Cpu cpu = {0};

  const bool program_loaded = cpu_load_program(
    &cpu,
    program_bytes,
    program_size
  );

  assert(program_loaded);

  const CpuRunResult run_result = cpu_run(
    &cpu,
    instruction_limit
  );

  assert(run_result == CPU_RUN_HALTED);
  assert(cpu.halted);
  assert(cpu.register_a == expected_register_a);
  assert(cpu.register_b == expected_register_b);
  assert(cpu.stack_pointer == CPU_STACK_EMPTY_POINTER);
  assert(!cpu.zero_flag);
  assert(!cpu.carry_flag);

  assert(
    cpu_read_memory(&cpu, data_address) ==
    expected_register_a
  );

  assert(
    cpu_read_memory(&cpu, initial_data_address) ==
    expected_register_a
  );

  assert(
    cpu.program_counter ==
    expected_program_counter
  );

  assert(cpu.cycle_count == expected_cycle_count);
}

int main(void)
{
  test_executes_assembled_program();

  puts("All assembled-program tests passed.");

  return EXIT_SUCCESS;
}
