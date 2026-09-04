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
  POPCOUNT_EXPECTED_PROGRAM_SIZE = 44
};

typedef struct PopcountTestCase
{
  uint8_t input_value;
  uint8_t expected_count;
} PopcountTestCase;

static const char POPCOUNT_PROGRAM_PATH[] =
  "build/popcount.bin";

static const uint8_t POPCOUNT_FINAL_PROGRAM_COUNTER =
  UINT8_C(0x1C);

static const uint8_t POPCOUNT_WORK_VALUE_ADDRESS =
  UINT8_C(0xD0);

static const uint8_t POPCOUNT_BIT_COUNT_ADDRESS =
  UINT8_C(0xD1);

static const uint8_t POPCOUNT_BITS_REMAINING_ADDRESS =
  UINT8_C(0xD2);

static const uint8_t POPCOUNT_REGISTER_B_VALUE =
  UINT8_C(0x01);

static const uint64_t POPCOUNT_BASE_CYCLE_COUNT =
  UINT64_C(114);

static const uint64_t POPCOUNT_EXTRA_CYCLES_PER_SET_BIT =
  UINT64_C(3);

static uint64_t popcount_expected_cycle_count(
    uint8_t set_bit_count
)
{
  return
    POPCOUNT_BASE_CYCLE_COUNT +
    (POPCOUNT_EXTRA_CYCLES_PER_SET_BIT *
     (uint64_t)set_bit_count);
}

static void test_executes_popcount_program(void)
{
  static const PopcountTestCase test_cases[] =
  {
    {UINT8_C(0x00), UINT8_C(0x00)},
    {UINT8_C(0xA5), UINT8_C(0x04)},
    {UINT8_C(0xFF), UINT8_C(0x08)}
  };

  uint8_t program_bytes[CPU_PROGRAM_MEMORY_SIZE] = {0};
  size_t program_size = 0;

  const bool read_succeeded = binary_reader_read(
    POPCOUNT_PROGRAM_PATH,
    program_bytes,
    sizeof program_bytes,
    &program_size
  );

  assert(read_succeeded);
  assert(program_size == POPCOUNT_EXPECTED_PROGRAM_SIZE);

  const size_t test_case_count =
    sizeof test_cases / sizeof test_cases[0];

  for (size_t index = 0; index < test_case_count; ++index)
  {
    const PopcountTestCase *const test_case =
      &test_cases[index];

    Cpu cpu = {0};

    const bool program_loaded = cpu_load_program(
      &cpu,
      program_bytes,
      program_size
    );

    assert(program_loaded);

    cpu_set_input_port(&cpu, test_case->input_value);

    const CpuRunResult run_result = cpu_run(
      &cpu,
      (uint64_t)CPU_MEMORY_SIZE
    );

    assert(run_result == CPU_RUN_HALTED);
    assert(cpu.halted);
    assert(cpu.register_a == test_case->expected_count);
    assert(cpu.register_b == POPCOUNT_REGISTER_B_VALUE);
    assert(cpu.stack_pointer == CPU_STACK_EMPTY_POINTER);
    assert(cpu.input_port == test_case->input_value);
    assert(cpu_get_output_port(&cpu) == test_case->expected_count);

    assert(
      cpu.zero_flag ==
      (test_case->expected_count == UINT8_C(0x00))
    );

    assert(!cpu.carry_flag);
    assert(cpu.program_counter == POPCOUNT_FINAL_PROGRAM_COUNTER);

    assert(
      cpu.cycle_count ==
      popcount_expected_cycle_count(test_case->expected_count)
    );

    assert(
      cpu_read_memory(&cpu, POPCOUNT_WORK_VALUE_ADDRESS) ==
      UINT8_C(0x00)
    );

    assert(
      cpu_read_memory(&cpu, POPCOUNT_BIT_COUNT_ADDRESS) ==
      test_case->expected_count
    );

    assert(
      cpu_read_memory(
        &cpu,
        POPCOUNT_BITS_REMAINING_ADDRESS
      ) == UINT8_C(0x00)
    );
  }
}

int main(void)
{
  test_executes_popcount_program();

  puts("All popcount-program tests passed.");

  return EXIT_SUCCESS;
}
