#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "cpu.h"

static void test_cpu_reset_clears_state(void)
{
  Cpu cpu = {
    .register_a = UINT8_MAX,
    .register_b = UINT8_MAX,
    .program_counter = UINT8_MAX,
    .zero_flag = true,
    .carry_flag = true,
    .halted = true,
    .cycle_count = UINT64_MAX
  };

  for (size_t index = 0; index < sizeof cpu.memory; ++index)
  {
    cpu.memory[index] = UINT8_MAX;
  }

  cpu_reset(&cpu);

  assert(cpu.register_a == 0);
  assert(cpu.register_b == 0);
  assert(cpu.program_counter == 0);
  assert(!cpu.zero_flag);
  assert(!cpu.carry_flag);
  assert(!cpu.halted);
  assert(cpu.cycle_count == 0);

  for (size_t index = 0; index < sizeof cpu.memory; ++index)
  {
    assert(cpu.memory[index] == 0);
  }
}

int main(void)
{
  test_cpu_reset_clears_state();

  puts("All tests passed.");
  return 0;
}
