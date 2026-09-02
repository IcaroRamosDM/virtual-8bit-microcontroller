#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "cpu.h"
#include "program.h"

static void test_demo_program_executes_expected_branch(void)
{
  const size_t expected_program_size = 10;
  const uint8_t expected_register_a = 0;
  const uint8_t expected_register_b = UINT8_C(0x2A);
  const uint64_t instruction_limit = CPU_MEMORY_SIZE;
  const uint64_t expected_cycle_count = UINT64_C(5);
  const Program program = program_get_demo();
  Cpu cpu = {0};

  assert(program.bytes != NULL);
  assert(program.size == expected_program_size);

  const bool loaded = cpu_load_program(
    &cpu,
    program.bytes,
    program.size
  );

  assert(loaded);

  const CpuRunResult result = cpu_run(
    &cpu,
    instruction_limit
  );

  assert(result == CPU_RUN_HALTED);
  assert(cpu.halted);
  assert(cpu.register_a == expected_register_a);
  assert(cpu.register_b == expected_register_b);
  assert(cpu.zero_flag);
  assert(!cpu.carry_flag);
  assert(
    cpu.program_counter == (uint8_t)expected_program_size
  );
  assert(cpu.cycle_count == expected_cycle_count);
}

int main(void)
{
  test_demo_program_executes_expected_branch();

  puts("All program tests passed.");

  return 0;
}
