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

static void test_cpu_load_program_copies_valid_program(void)
{
  const uint8_t program[] = {
    OPCODE_NOP,
    OPCODE_HALT
  };
  const size_t program_size = sizeof program / sizeof program[0];
  const uint8_t first_unwritten_address = (uint8_t)program_size;
  Cpu cpu = {0};

  const bool loaded = cpu_load_program(&cpu, program, program_size);

  assert(loaded);

  for (size_t index = 0; index < program_size; ++index)
  {
    assert(
      cpu_read_memory(&cpu, (uint8_t)index) == program[index]
    );
  }

  assert(cpu_read_memory(&cpu, first_unwritten_address) == 0);
  assert(cpu.program_counter == 0);
}

static void test_cpu_load_program_accepts_full_memory(void)
{
  const uint8_t last_address = UINT8_MAX;
  uint8_t program[CPU_MEMORY_SIZE] = {0};
  Cpu cpu = {0};

  program[last_address] = OPCODE_HALT;

  const bool loaded = cpu_load_program(
    &cpu,
    program,
    sizeof program
  );

  assert(loaded);
  assert(cpu_read_memory(&cpu, last_address) == OPCODE_HALT);
}

static void test_cpu_load_program_validates_input(void)
{
  enum
  {
    OVERSIZED_PROGRAM_SIZE = CPU_MEMORY_SIZE + 1
  };

  const uint8_t oversized_program[OVERSIZED_PROGRAM_SIZE] = {0};
  const uint8_t sentinel_address = 0;
  const uint8_t sentinel_value = UINT8_C(0xA5);
  const size_t nonempty_program_size = 1;
  Cpu cpu = {0};

  cpu_write_memory(&cpu, sentinel_address, sentinel_value);

  const bool oversized_program_loaded = cpu_load_program(
    &cpu,
    oversized_program,
    sizeof oversized_program
  );

  assert(!oversized_program_loaded);
  assert(
    cpu_read_memory(&cpu, sentinel_address) == sentinel_value
  );

  const bool null_program_loaded = cpu_load_program(
    &cpu,
    NULL,
    nonempty_program_size
  );

  assert(!null_program_loaded);
  assert(
    cpu_read_memory(&cpu, sentinel_address) == sentinel_value
  );

  const bool empty_program_loaded = cpu_load_program(
    &cpu,
    NULL,
    0
  );

  assert(empty_program_loaded);
  assert(
    cpu_read_memory(&cpu, sentinel_address) == sentinel_value
  );
}

static void test_cpu_run_stops_at_halt(void)
{
  const uint8_t program[] = {
    OPCODE_NOP,
    OPCODE_NOP,
    OPCODE_HALT
  };
  const size_t program_size = sizeof program / sizeof program[0];
  const uint64_t instruction_limit = CPU_MEMORY_SIZE;
  const uint64_t expected_cycle_count = (uint64_t)program_size;
  const uint8_t expected_program_counter = (uint8_t)program_size;
  Cpu cpu = {0};

  const bool loaded = cpu_load_program(
    &cpu,
    program,
    program_size
  );

  assert(loaded);

  const CpuRunResult result = cpu_run(
    &cpu,
    instruction_limit
  );

  assert(result == CPU_RUN_HALTED);
  assert(cpu.halted);
  assert(cpu.cycle_count == expected_cycle_count);
  assert(cpu.program_counter == expected_program_counter);
}

static void test_cpu_run_reports_invalid_opcode(void)
{
  const uint8_t program[] = {
    UINT8_MAX
  };
  const size_t program_size = sizeof program / sizeof program[0];
  const uint64_t instruction_limit = (uint64_t)program_size;
  const uint64_t expected_cycle_count = (uint64_t)program_size;
  const uint8_t expected_program_counter = (uint8_t)program_size;
  Cpu cpu = {0};

  const bool loaded = cpu_load_program(
    &cpu,
    program,
    program_size
  );

  assert(loaded);

  const CpuRunResult result = cpu_run(
    &cpu,
    instruction_limit
  );

  assert(result == CPU_RUN_INVALID_OPCODE);
  assert(cpu.halted);
  assert(cpu.cycle_count == expected_cycle_count);
  assert(cpu.program_counter == expected_program_counter);
}

static void test_cpu_run_enforces_instruction_limit(void)
{
  const uint8_t program[] = {
    OPCODE_NOP,
    OPCODE_NOP,
    OPCODE_NOP
  };
  const size_t program_size = sizeof program / sizeof program[0];
  const uint64_t instruction_limit = UINT64_C(2);
  const uint64_t expected_cycle_count = instruction_limit;
  const uint8_t expected_program_counter =
    (uint8_t)instruction_limit;
  Cpu cpu = {0};

  const bool loaded = cpu_load_program(
    &cpu,
    program,
    program_size
  );

  assert(loaded);

  const CpuRunResult result = cpu_run(
    &cpu,
    instruction_limit
  );

  assert(result == CPU_RUN_INSTRUCTION_LIMIT_REACHED);
  assert(!cpu.halted);
  assert(cpu.cycle_count == expected_cycle_count);
  assert(cpu.program_counter == expected_program_counter);
}

static void test_cpu_run_handles_zero_limit_and_halted_cpu(void)
{
  const uint64_t zero_instruction_limit = 0;
  Cpu active_cpu = {0};
  Cpu halted_cpu = {
    .halted = true
  };

  const CpuRunResult active_result = cpu_run(
    &active_cpu,
    zero_instruction_limit
  );

  const CpuRunResult halted_result = cpu_run(
    &halted_cpu,
    zero_instruction_limit
  );

  assert(
    active_result == CPU_RUN_INSTRUCTION_LIMIT_REACHED
  );
  assert(active_cpu.program_counter == 0);
  assert(active_cpu.cycle_count == 0);
  assert(!active_cpu.halted);

  assert(halted_result == CPU_RUN_HALTED);
  assert(halted_cpu.program_counter == 0);
  assert(halted_cpu.cycle_count == 0);
  assert(halted_cpu.halted);
}

int main(void)
{
  test_cpu_reset_clears_state();
  test_cpu_memory_read_and_write();
  test_cpu_fetch_byte_reads_and_advances_program_counter();
  test_cpu_step_executes_nop_and_halt();
  test_cpu_step_rejects_invalid_opcode();
  test_cpu_load_program_copies_valid_program();
  test_cpu_run_stops_at_halt();
  test_cpu_run_reports_invalid_opcode();
  test_cpu_run_enforces_instruction_limit();
  test_cpu_run_handles_zero_limit_and_halted_cpu();
  test_cpu_load_program_accepts_full_memory();
  test_cpu_load_program_validates_input();

  puts("All tests passed.");

  return 0;
}
