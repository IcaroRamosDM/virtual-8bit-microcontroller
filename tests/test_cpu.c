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
  const uint8_t last_address_value = UINT8_C(0x5A);
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

  const uint8_t first_fetched_value = cpu_fetch_byte(&cpu);

  assert(first_fetched_value == first_value);
  assert(cpu.program_counter == second_address);

  const uint8_t second_fetched_value = cpu_fetch_byte(&cpu);

  assert(second_fetched_value == second_value);
  assert(cpu.program_counter == second_address + 1);

  cpu.program_counter = last_address;

  const uint8_t last_fetched_value = cpu_fetch_byte(&cpu);

  assert(last_fetched_value == last_value);
  assert(cpu.program_counter == 0);
}

static void test_cpu_step_executes_nop_and_halt(void)
{
  const uint8_t program_start = 0;
  const uint8_t halt_address = program_start + 1;
  const uint8_t address_after_halt = halt_address + 1;
  const uint64_t cycles_after_nop = UINT64_C(1);
  const uint64_t cycles_after_halt = cycles_after_nop + 1;
  Cpu cpu = {0};

  cpu_write_memory(&cpu, program_start, OPCODE_NOP);
  cpu_write_memory(&cpu, halt_address, OPCODE_HALT);

  const CpuStepResult nop_result = cpu_step(&cpu);

  assert(nop_result == CPU_STEP_OK);
  assert(cpu.program_counter == halt_address);
  assert(cpu.cycle_count == cycles_after_nop);
  assert(!cpu.halted);

  const CpuStepResult halt_result = cpu_step(&cpu);

  assert(halt_result == CPU_STEP_HALTED);
  assert(cpu.program_counter == address_after_halt);
  assert(cpu.cycle_count == cycles_after_halt);
  assert(cpu.halted);

  const CpuStepResult halted_result = cpu_step(&cpu);

  assert(halted_result == CPU_STEP_HALTED);
  assert(cpu.program_counter == address_after_halt);
  assert(cpu.cycle_count == cycles_after_halt);
}

static void test_cpu_step_rejects_invalid_opcode(void)
{
  const uint8_t program_start = 0;
  const uint8_t address_after_invalid_opcode = program_start + 1;
  const uint8_t invalid_opcode = UINT8_MAX;
  const uint64_t expected_cycle_count = UINT64_C(1);
  Cpu cpu = {0};

  cpu_write_memory(&cpu, program_start, invalid_opcode);

  const CpuStepResult invalid_opcode_result = cpu_step(&cpu);

  assert(invalid_opcode_result == CPU_STEP_INVALID_OPCODE);
  assert(cpu.program_counter == address_after_invalid_opcode);
  assert(cpu.cycle_count == expected_cycle_count);
  assert(cpu.halted);
}

int main(void)
{
  test_cpu_reset_clears_state();
  test_cpu_memory_read_and_write();
  test_cpu_fetch_byte_reads_and_advances_program_counter();
  test_cpu_step_executes_nop_and_halt();
  test_cpu_step_rejects_invalid_opcode();

  puts("All tests passed.");

  return 0;
}
