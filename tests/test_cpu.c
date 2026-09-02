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

static void test_cpu_memory_read_and_write(void)
{
  const uint8_t first_address = 0;
  const uint8_t middle_address = CPU_MEMORY_SIZE / 2;
  const uint8_t last_address = UINT8_MAX;
  const uint8_t first_address_value = UINT8_C(0xA5);
  const uint8_t last_address_value = UINT8_C(0x5Al);
  Cpu cpu = {0};

  cpu_write_memory(&cpu, first_address, first_address_value);
  cpu_write_memory(&cpu, last_address, last_address_value);

  assert(cpu_read_memory(&cpu, first_address) == first_address_value);
  assert(cpu_read_memory(&cpu, last_address) == last_address_value);
  assert(cpu_read_memory(&cpu, middle_address) == 0);
}

static void test_cpu_fetch_byte_reads_and_advances_program_counter(void)
{
  const uint8_t first_address = 0;
  const uint8_t second_address = first_address + 1;
  const uint8_t last_address = UINT8_MAX;
  const uint8_t first_value = UINT8_C(0xA5);
  const uint8_t second_value = UINT8_C(0x5A);
  const uint8_t last_value = UINT8_C(0x3C);
  Cpu cpu = {0};

  cpu_write_memory(&cpu, first_address, first_value);
  cpu_write_memory(&cpu, second_address, second_value);
  cpu_write_memory(&cpu, last_address, last_value);

  assert(cpu_fetch_byte(&cpu) == first_value);
  assert(cpu.program_counter == second_address);

  assert(cpu_fetch_byte(&cpu) == second_value);
  assert(cpu.program_counter == second_address + 1);

  cpu.program_counter = last_address;

  assert(cpu_fetch_byte(&cpu) == last_value);
  assert(cpu.program_counter == 0);
}

int main(void)
{
  test_cpu_reset_clears_state();
  test_cpu_memory_read_and_write();
  test_cpu_fetch_byte_reads_and_advances_program_counter();

  puts("All tests passed.");

  return 0;
}

