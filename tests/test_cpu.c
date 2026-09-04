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
    .stack_pointer = UINT8_MAX,
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
  assert(cpu.stack_pointer == CPU_STACK_EMPTY_POINTER);
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

static void test_cpu_load_program_accepts_full_program_region(void)
{
  const uint8_t last_program_address =
    (uint8_t)(CPU_PROGRAM_MEMORY_SIZE - 1);
  uint8_t program[CPU_PROGRAM_MEMORY_SIZE] = {0};
  Cpu cpu = {0};

  program[last_program_address] = OPCODE_HALT;

  const bool loaded = cpu_load_program(
    &cpu,
    program,
    sizeof program
  );

  assert(loaded);
  assert(
    cpu_read_memory(&cpu, last_program_address) ==
    OPCODE_HALT
  );
  assert(
    cpu_read_memory(&cpu, CPU_STACK_LOW_ADDRESS) == 0
  );
}

static void test_cpu_load_program_validates_input(void)
{
  enum
  {
    OVERSIZED_PROGRAM_SIZE = CPU_PROGRAM_MEMORY_SIZE + 1
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
  const uint8_t program_start = 0;
  const uint8_t program[] = {
    OPCODE_JUMP,
    program_start
  };
  const size_t program_size = sizeof program / sizeof program[0];
  const uint64_t instruction_limit = UINT64_C(3);
  const uint64_t expected_cycle_count = instruction_limit;
  const uint8_t expected_program_counter = program_start;
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

static void test_cpu_step_executes_load_immediate_a(void)
{
  const uint8_t nonzero_value = UINT8_C(0xA5);
  const uint8_t nonzero_program[] = {
    OPCODE_LOAD_IMMEDIATE_A,
    nonzero_value
  };
  const size_t nonzero_program_size =
    sizeof nonzero_program / sizeof nonzero_program[0];

  const uint8_t zero_value = 0;
  const uint8_t zero_program[] = {
    OPCODE_LOAD_IMMEDIATE_A,
    zero_value
  };
  const size_t zero_program_size =
    sizeof zero_program / sizeof zero_program[0];

  const uint64_t expected_cycle_count = UINT64_C(1);

  Cpu nonzero_cpu = {
    .zero_flag = true,
    .carry_flag = true
  };
  Cpu zero_cpu = {
    .register_a = UINT8_MAX,
    .carry_flag = true
  };

  const bool nonzero_program_loaded = cpu_load_program(
    &nonzero_cpu,
    nonzero_program,
    nonzero_program_size
  );
  const bool zero_program_loaded = cpu_load_program(
    &zero_cpu,
    zero_program,
    zero_program_size
  );

  assert(nonzero_program_loaded);
  assert(zero_program_loaded);

  const CpuStepResult nonzero_result = cpu_step(&nonzero_cpu);
  const CpuStepResult zero_result = cpu_step(&zero_cpu);

  assert(nonzero_result == CPU_STEP_OK);
  assert(nonzero_cpu.register_a == nonzero_value);
  assert(!nonzero_cpu.zero_flag);
  assert(nonzero_cpu.carry_flag);
  assert(!nonzero_cpu.halted);
  assert(
    nonzero_cpu.program_counter == (uint8_t)nonzero_program_size
  );
  assert(nonzero_cpu.cycle_count == expected_cycle_count);

  assert(zero_result == CPU_STEP_OK);
  assert(zero_cpu.register_a == zero_value);
  assert(zero_cpu.zero_flag);
  assert(zero_cpu.carry_flag);
  assert(!zero_cpu.halted);
  assert(zero_cpu.program_counter == (uint8_t)zero_program_size);
  assert(zero_cpu.cycle_count == expected_cycle_count);
}

static void test_cpu_step_executes_load_immediate_b(void)
{
  const uint8_t nonzero_value = UINT8_C(0x5A);
  const uint8_t nonzero_program[] = {
    OPCODE_LOAD_IMMEDIATE_B,
    nonzero_value
  };
  const size_t nonzero_program_size =
    sizeof nonzero_program / sizeof nonzero_program[0];

  const uint8_t zero_value = 0;
  const uint8_t zero_program[] = {
    OPCODE_LOAD_IMMEDIATE_B,
    zero_value
  };
  const size_t zero_program_size =
    sizeof zero_program / sizeof zero_program[0];

  const uint64_t expected_cycle_count = UINT64_C(1);

  Cpu nonzero_cpu = {
    .zero_flag = true,
    .carry_flag = true
  };
  Cpu zero_cpu = {
    .register_b = UINT8_MAX,
    .carry_flag = true
  };

  const bool nonzero_program_loaded = cpu_load_program(
    &nonzero_cpu,
    nonzero_program,
    nonzero_program_size
  );
  const bool zero_program_loaded = cpu_load_program(
    &zero_cpu,
    zero_program,
    zero_program_size
  );

  assert(nonzero_program_loaded);
  assert(zero_program_loaded);

  const CpuStepResult nonzero_result = cpu_step(&nonzero_cpu);
  const CpuStepResult zero_result = cpu_step(&zero_cpu);

  assert(nonzero_result == CPU_STEP_OK);
  assert(nonzero_cpu.register_b == nonzero_value);
  assert(!nonzero_cpu.zero_flag);
  assert(nonzero_cpu.carry_flag);
  assert(!nonzero_cpu.halted);
  assert(
    nonzero_cpu.program_counter == (uint8_t)nonzero_program_size
  );
  assert(nonzero_cpu.cycle_count == expected_cycle_count);

  assert(zero_result == CPU_STEP_OK);
  assert(zero_cpu.register_b == zero_value);
  assert(zero_cpu.zero_flag);
  assert(zero_cpu.carry_flag);
  assert(!zero_cpu.halted);
  assert(zero_cpu.program_counter == (uint8_t)zero_program_size);
  assert(zero_cpu.cycle_count == expected_cycle_count);
}

static void test_cpu_step_executes_add_a_b(void)
{
  typedef struct AddTestCase
  {
    uint8_t initial_a;
    uint8_t initial_b;
    uint8_t expected_a;
    bool expected_zero_flag;
    bool expected_carry_flag;
  } AddTestCase;

  const AddTestCase test_cases[] = {
    {
      .initial_a = UINT8_C(0x12),
      .initial_b = UINT8_C(0x34),
      .expected_a = UINT8_C(0x46),
      .expected_zero_flag = false,
      .expected_carry_flag = false
    },
    {
      .initial_a = UINT8_C(0xF0),
      .initial_b = UINT8_C(0x20),
      .expected_a = UINT8_C(0x10),
      .expected_zero_flag = false,
      .expected_carry_flag = true
    },
    {
      .initial_a = UINT8_C(0xFF),
      .initial_b = UINT8_C(0x01),
      .expected_a = 0,
      .expected_zero_flag = true,
      .expected_carry_flag = true
    },
    {
      .initial_a = 0,
      .initial_b = 0,
      .expected_a = 0,
      .expected_zero_flag = true,
      .expected_carry_flag = false
    }
  };

  const size_t test_case_count =
    sizeof test_cases / sizeof test_cases[0];
  const uint8_t instruction_address = 0;
  const uint8_t expected_program_counter =
    instruction_address + 1;
  const uint64_t expected_cycle_count = UINT64_C(1);

  for (size_t index = 0; index < test_case_count; ++index)
  {
    const AddTestCase *test_case = &test_cases[index];

    Cpu cpu = {
      .register_a = test_case->initial_a,
      .register_b = test_case->initial_b,
      .zero_flag = !test_case->expected_zero_flag,
      .carry_flag = !test_case->expected_carry_flag
    };

    cpu_write_memory(
      &cpu,
      instruction_address,
      OPCODE_ADD_A_B
    );

    const CpuStepResult result = cpu_step(&cpu);

    assert(result == CPU_STEP_OK);
    assert(cpu.register_a == test_case->expected_a);
    assert(cpu.register_b == test_case->initial_b);
    assert(cpu.zero_flag == test_case->expected_zero_flag);
    assert(cpu.carry_flag == test_case->expected_carry_flag);
    assert(cpu.program_counter == expected_program_counter);
    assert(cpu.cycle_count == expected_cycle_count);
    assert(!cpu.halted);
  }
}

static void test_cpu_step_executes_sub_a_b(void)
{
  typedef struct SubtractTestCase
  {
    uint8_t initial_a;
    uint8_t initial_b;
    uint8_t expected_a;
    bool expected_zero_flag;
    bool expected_carry_flag;
  } SubtractTestCase;

  const SubtractTestCase test_cases[] = {
    {
      .initial_a = UINT8_C(0x34),
      .initial_b = UINT8_C(0x12),
      .expected_a = UINT8_C(0x22),
      .expected_zero_flag = false,
      .expected_carry_flag = false
    },
    {
      .initial_a = UINT8_C(0x12),
      .initial_b = UINT8_C(0x34),
      .expected_a = UINT8_C(0xDE),
      .expected_zero_flag = false,
      .expected_carry_flag = true
    },
    {
      .initial_a = UINT8_C(0x5A),
      .initial_b = UINT8_C(0x5A),
      .expected_a = 0,
      .expected_zero_flag = true,
      .expected_carry_flag = false
    },
    {
      .initial_a = 0,
      .initial_b = UINT8_C(0x01),
      .expected_a = UINT8_MAX,
      .expected_zero_flag = false,
      .expected_carry_flag = true
    }
  };

  const size_t test_case_count =
    sizeof test_cases / sizeof test_cases[0];
  const uint8_t instruction_address = 0;
  const uint8_t expected_program_counter =
    instruction_address + 1;
  const uint64_t expected_cycle_count = UINT64_C(1);

  for (size_t index = 0; index < test_case_count; ++index)
  {
    const SubtractTestCase *test_case = &test_cases[index];

    Cpu cpu = {
      .register_a = test_case->initial_a,
      .register_b = test_case->initial_b,
      .zero_flag = !test_case->expected_zero_flag,
      .carry_flag = !test_case->expected_carry_flag
    };

    cpu_write_memory(
      &cpu,
      instruction_address,
      OPCODE_SUB_A_B
    );

    const CpuStepResult result = cpu_step(&cpu);

    assert(result == CPU_STEP_OK);
    assert(cpu.register_a == test_case->expected_a);
    assert(cpu.register_b == test_case->initial_b);
    assert(cpu.zero_flag == test_case->expected_zero_flag);
    assert(cpu.carry_flag == test_case->expected_carry_flag);
    assert(cpu.program_counter == expected_program_counter);
    assert(cpu.cycle_count == expected_cycle_count);
    assert(!cpu.halted);
  }
}

static void test_cpu_step_executes_bitwise_operations(void)
{
  typedef struct BitwiseTestCase
  {
    Opcode opcode;
    uint8_t initial_a;
    uint8_t initial_b;
    uint8_t expected_a;
    bool expected_carry_flag;
  } BitwiseTestCase;

  static const BitwiseTestCase test_cases[] =
  {
    {
      OPCODE_AND_A_B,
      UINT8_C(0xF0),
      UINT8_C(0x3C),
      UINT8_C(0x30),
      false
    },
    {
      OPCODE_AND_A_B,
      UINT8_C(0x0F),
      UINT8_C(0xF0),
      UINT8_C(0x00),
      false
    },
    {
      OPCODE_OR_A_B,
      UINT8_C(0x50),
      UINT8_C(0x0A),
      UINT8_C(0x5A),
      false
    },
    {
      OPCODE_OR_A_B,
      UINT8_C(0x00),
      UINT8_C(0x00),
      UINT8_C(0x00),
      false
    },
    {
      OPCODE_XOR_A_B,
      UINT8_C(0xAA),
      UINT8_C(0x0F),
      UINT8_C(0xA5),
      false
    },
    {
      OPCODE_XOR_A_B,
      UINT8_C(0x5A),
      UINT8_C(0x5A),
      UINT8_C(0x00),
      false
    },
    {
      OPCODE_NOT_A,
      UINT8_C(0x0F),
      UINT8_C(0xA5),
      UINT8_C(0xF0),
      false
    },
    {
      OPCODE_NOT_A,
      UINT8_C(0xFF),
      UINT8_C(0xA5),
      UINT8_C(0x00),
      false
    },
    {
      OPCODE_SHIFT_LEFT_A,
      UINT8_C(0x25),
      UINT8_C(0xA5),
      UINT8_C(0x4A),
      false
    },
    {
      OPCODE_SHIFT_LEFT_A,
      UINT8_C(0x80),
      UINT8_C(0xA5),
      UINT8_C(0x00),
      true
    },
    {
      OPCODE_SHIFT_RIGHT_A,
      UINT8_C(0x84),
      UINT8_C(0xA5),
      UINT8_C(0x42),
      false
    },
    {
      OPCODE_SHIFT_RIGHT_A,
      UINT8_C(0x01),
      UINT8_C(0xA5),
      UINT8_C(0x00),
      true
    }
  };

  const size_t test_case_count =
    sizeof test_cases / sizeof test_cases[0];
  const uint8_t instruction_address = 0;
  const uint8_t expected_program_counter =
    instruction_address + 1;
  const uint64_t expected_cycle_count = UINT64_C(1);

  for (
    size_t index = 0;
    index < test_case_count;
    ++index
  )
  {
    const BitwiseTestCase *const test_case =
      &test_cases[index];

    const bool expected_zero_flag =
      test_case->expected_a == 0;

    Cpu cpu =
    {
      .register_a = test_case->initial_a,
      .register_b = test_case->initial_b,
      .zero_flag = !expected_zero_flag,
      .carry_flag = !test_case->expected_carry_flag
    };

    cpu_write_memory(
      &cpu,
      instruction_address,
      (uint8_t)test_case->opcode
    );

    const CpuStepResult result = cpu_step(&cpu);

    assert(result == CPU_STEP_OK);
    assert(cpu.register_a == test_case->expected_a);
    assert(cpu.register_b == test_case->initial_b);
    assert(cpu.zero_flag == expected_zero_flag);
    assert(
      cpu.carry_flag ==
      test_case->expected_carry_flag
    );
    assert(cpu.program_counter == expected_program_counter);
    assert(cpu.cycle_count == expected_cycle_count);
    assert(!cpu.halted);
  }
}

static void test_cpu_step_executes_compare_a_b(void)
{
  typedef struct CompareTestCase
  {
    uint8_t register_a;
    uint8_t register_b;
    bool expected_zero_flag;
    bool expected_carry_flag;
  } CompareTestCase;

  static const CompareTestCase test_cases[] =
  {
    {
      UINT8_C(0x5A),
      UINT8_C(0x5A),
      true,
      false
    },
    {
      UINT8_C(0x20),
      UINT8_C(0x10),
      false,
      false
    },
    {
      UINT8_C(0x10),
      UINT8_C(0x20),
      false,
      true
    }
  };

  const size_t test_case_count =
    sizeof test_cases / sizeof test_cases[0];
  const uint8_t instruction_address = 0;
  const uint8_t expected_program_counter =
    instruction_address + 1;
  const uint64_t expected_cycle_count = UINT64_C(1);

  for (
    size_t index = 0;
    index < test_case_count;
    ++index
  )
  {
    const CompareTestCase *const test_case =
      &test_cases[index];

    Cpu cpu =
    {
      .register_a = test_case->register_a,
      .register_b = test_case->register_b,
      .zero_flag = !test_case->expected_zero_flag,
      .carry_flag = !test_case->expected_carry_flag
    };

    cpu_write_memory(
      &cpu,
      instruction_address,
      (uint8_t)OPCODE_COMPARE_A_B
    );

    const CpuStepResult result = cpu_step(&cpu);

    assert(result == CPU_STEP_OK);
    assert(cpu.register_a == test_case->register_a);
    assert(cpu.register_b == test_case->register_b);
    assert(cpu.zero_flag == test_case->expected_zero_flag);
    assert(
      cpu.carry_flag ==
      test_case->expected_carry_flag
    );
    assert(cpu.program_counter == expected_program_counter);
    assert(cpu.cycle_count == expected_cycle_count);
    assert(!cpu.halted);
  }
}

static void test_cpu_step_executes_conditional_branches(void)
{
  typedef struct ConditionalBranchTestCase
  {
    Opcode opcode;
    bool zero_flag;
    bool carry_flag;
    bool expected_taken;
  } ConditionalBranchTestCase;

  static const ConditionalBranchTestCase test_cases[] =
  {
    {OPCODE_JUMP_IF_ZERO, true, false, true},
    {OPCODE_JUMP_IF_ZERO, false, true, false},
    {OPCODE_JUMP_IF_NOT_ZERO, false, false, true},
    {OPCODE_JUMP_IF_NOT_ZERO, true, true, false},
    {OPCODE_JUMP_IF_CARRY, true, true, true},
    {OPCODE_JUMP_IF_CARRY, false, true, true},
    {OPCODE_JUMP_IF_CARRY, true, false, false},
    {OPCODE_JUMP_IF_CARRY, false, false, false}
  };

  const size_t test_case_count =
    sizeof test_cases / sizeof test_cases[0];
  const uint8_t target_address = UINT8_C(0x80);
  const uint8_t expected_register_a = UINT8_C(0xA5);
  const uint8_t expected_register_b = UINT8_C(0x5A);
  const uint64_t expected_cycle_count = UINT64_C(1);

  for (
    size_t index = 0;
    index < test_case_count;
    ++index
  )
  {
    const ConditionalBranchTestCase *const test_case =
      &test_cases[index];
    const uint8_t program[] =
    {
      (uint8_t)test_case->opcode,
      target_address
    };
    const size_t program_size =
      sizeof program / sizeof program[0];
    const uint8_t expected_program_counter =
      test_case->expected_taken
        ? target_address
        : (uint8_t)program_size;

    Cpu cpu =
    {
      .register_a = expected_register_a,
      .register_b = expected_register_b,
      .zero_flag = test_case->zero_flag,
      .carry_flag = test_case->carry_flag
    };

    const bool program_loaded = cpu_load_program(
      &cpu,
      program,
      program_size
    );

    assert(program_loaded);

    const CpuStepResult result = cpu_step(&cpu);

    assert(result == CPU_STEP_OK);
    assert(cpu.program_counter == expected_program_counter);
    assert(cpu.cycle_count == expected_cycle_count);
    assert(cpu.register_a == expected_register_a);
    assert(cpu.register_b == expected_register_b);
    assert(cpu.zero_flag == test_case->zero_flag);
    assert(cpu.carry_flag == test_case->carry_flag);
    assert(!cpu.halted);
  }
}

static void test_cpu_step_executes_jump(void)
{
  const uint8_t target_address = UINT8_C(0x80);
  const uint8_t expected_register_a = UINT8_C(0xA5);
  const uint8_t expected_register_b = UINT8_C(0x5A);
  const uint8_t program[] = {
    OPCODE_JUMP,
    target_address
  };
  const size_t program_size =
    sizeof program / sizeof program[0];
  const uint64_t expected_cycle_count = UINT64_C(1);

  Cpu cpu = {
    .register_a = expected_register_a,
    .register_b = expected_register_b,
    .zero_flag = true,
    .carry_flag = true
  };

  const bool loaded = cpu_load_program(
    &cpu,
    program,
    program_size
  );

  assert(loaded);

  const CpuStepResult result = cpu_step(&cpu);

  assert(result == CPU_STEP_OK);
  assert(cpu.program_counter == target_address);
  assert(cpu.cycle_count == expected_cycle_count);
  assert(cpu.register_a == expected_register_a);
  assert(cpu.register_b == expected_register_b);
  assert(cpu.zero_flag);
  assert(cpu.carry_flag);
  assert(!cpu.halted);
}

static void test_cpu_step_executes_load_a_from_memory(void)
{
  typedef struct LoadMemoryTestCase
  {
    uint8_t stored_value;
    bool expected_zero_flag;
  } LoadMemoryTestCase;

  const LoadMemoryTestCase test_cases[] = {
    {
      .stored_value = 0,
      .expected_zero_flag = true
    },
    {
      .stored_value = UINT8_C(0xA5),
      .expected_zero_flag = false
    }
  };

  const size_t test_case_count =
    sizeof test_cases / sizeof test_cases[0];
  const uint8_t data_address = UINT8_C(0x80);
  const uint8_t expected_register_b = UINT8_C(0x5A);
  const uint8_t program[] = {
    OPCODE_LOAD_A_FROM_MEMORY,
    data_address
  };
  const size_t program_size =
    sizeof program / sizeof program[0];
  const uint64_t expected_cycle_count = UINT64_C(1);

  for (size_t index = 0; index < test_case_count; ++index)
  {
    const LoadMemoryTestCase *test_case = &test_cases[index];

    Cpu cpu = {
      .register_a = UINT8_MAX,
      .register_b = expected_register_b,
      .zero_flag = !test_case->expected_zero_flag,
      .carry_flag = true
    };

    const bool loaded = cpu_load_program(
      &cpu,
      program,
      program_size
    );

    assert(loaded);

    cpu_write_memory(
      &cpu,
      data_address,
      test_case->stored_value
    );

    const CpuStepResult result = cpu_step(&cpu);

    assert(result == CPU_STEP_OK);
    assert(cpu.register_a == test_case->stored_value);
    assert(cpu.register_b == expected_register_b);
    assert(
      cpu.zero_flag == test_case->expected_zero_flag
    );
    assert(cpu.carry_flag);
    assert(
      cpu_read_memory(&cpu, data_address) ==
      test_case->stored_value
    );
    assert(cpu.program_counter == (uint8_t)program_size);
    assert(cpu.cycle_count == expected_cycle_count);
    assert(!cpu.halted);
  }
}

static void test_cpu_step_executes_store_a_to_memory(void)
{
  const uint8_t data_address = UINT8_C(0x80);
  const uint8_t initial_memory_value = UINT8_C(0x3C);
  const uint8_t value_to_store = UINT8_C(0xA5);
  const uint8_t expected_register_b = UINT8_C(0x5A);
  const uint8_t program[] = {
    OPCODE_STORE_A_TO_MEMORY,
    data_address
  };
  const size_t program_size =
    sizeof program / sizeof program[0];
  const uint64_t expected_cycle_count = UINT64_C(1);

  Cpu cpu = {
    .register_a = value_to_store,
    .register_b = expected_register_b,
    .zero_flag = true,
    .carry_flag = true
  };

  const bool loaded = cpu_load_program(
    &cpu,
    program,
    program_size
  );

  assert(loaded);

  cpu_write_memory(
    &cpu,
    data_address,
    initial_memory_value
  );

  const CpuStepResult result = cpu_step(&cpu);

  assert(result == CPU_STEP_OK);
  assert(
    cpu_read_memory(&cpu, data_address) == value_to_store
  );
  assert(cpu.register_a == value_to_store);
  assert(cpu.register_b == expected_register_b);
  assert(cpu.zero_flag);
  assert(cpu.carry_flag);
  assert(cpu.program_counter == (uint8_t)program_size);
  assert(cpu.cycle_count == expected_cycle_count);
  assert(!cpu.halted);
}

static void test_cpu_step_executes_push_a(void)
{
  const uint8_t first_value = UINT8_C(0xA5);
  const uint8_t second_value = UINT8_C(0x5A);
  const uint8_t expected_register_b = UINT8_C(0x3C);
  const uint8_t second_stack_address =
    (uint8_t)(CPU_STACK_HIGH_ADDRESS - 1);
  const uint8_t program[] = {
    OPCODE_PUSH_A,
    OPCODE_PUSH_A
  };
  const size_t program_size =
    sizeof program / sizeof program[0];
  const uint64_t expected_cycle_count =
    (uint64_t)program_size;

  Cpu cpu = {
    .register_a = first_value,
    .register_b = expected_register_b,
    .zero_flag = true,
    .carry_flag = true
  };

  const bool loaded = cpu_load_program(
    &cpu,
    program,
    program_size
  );

  assert(loaded);

  const CpuStepResult first_result = cpu_step(&cpu);

  assert(first_result == CPU_STEP_OK);
  assert(cpu.stack_pointer == CPU_STACK_HIGH_ADDRESS);
  assert(
    cpu_read_memory(&cpu, CPU_STACK_HIGH_ADDRESS) ==
    first_value
  );

  cpu.register_a = second_value;

  const CpuStepResult second_result = cpu_step(&cpu);

  assert(second_result == CPU_STEP_OK);
  assert(cpu.stack_pointer == second_stack_address);
  assert(
    cpu_read_memory(&cpu, second_stack_address) ==
    second_value
  );
  assert(
    cpu_read_memory(&cpu, CPU_STACK_HIGH_ADDRESS) ==
    first_value
  );
  assert(cpu.register_a == second_value);
  assert(cpu.register_b == expected_register_b);
  assert(cpu.zero_flag);
  assert(cpu.carry_flag);
  assert(cpu.program_counter == (uint8_t)program_size);
  assert(cpu.cycle_count == expected_cycle_count);
  assert(!cpu.halted);
}

static void test_cpu_step_executes_pop_a(void)
{
  const uint8_t older_value = UINT8_C(0xA5);
  const uint8_t newer_value = 0;
  const uint8_t expected_register_b = UINT8_C(0x3C);
  const uint8_t newer_value_address =
    (uint8_t)(CPU_STACK_HIGH_ADDRESS - 1);
  const uint8_t program[] = {
    OPCODE_POP_A,
    OPCODE_POP_A
  };
  const size_t program_size =
    sizeof program / sizeof program[0];
  const uint64_t expected_cycle_count =
    (uint64_t)program_size;

  Cpu cpu = {
    .register_a = UINT8_MAX,
    .register_b = expected_register_b,
    .stack_pointer = newer_value_address,
    .carry_flag = true
  };

  const bool loaded = cpu_load_program(
    &cpu,
    program,
    program_size
  );

  assert(loaded);

  cpu_write_memory(
    &cpu,
    CPU_STACK_HIGH_ADDRESS,
    older_value
  );

  cpu_write_memory(
    &cpu,
    newer_value_address,
    newer_value
  );

  const CpuStepResult first_result = cpu_step(&cpu);

  assert(first_result == CPU_STEP_OK);
  assert(cpu.register_a == newer_value);
  assert(cpu.zero_flag);
  assert(cpu.stack_pointer == CPU_STACK_HIGH_ADDRESS);

  const CpuStepResult second_result = cpu_step(&cpu);

  assert(second_result == CPU_STEP_OK);
  assert(cpu.register_a == older_value);
  assert(!cpu.zero_flag);
  assert(cpu.stack_pointer == CPU_STACK_EMPTY_POINTER);
  assert(cpu.register_b == expected_register_b);
  assert(cpu.carry_flag);
  assert(cpu.program_counter == (uint8_t)program_size);
  assert(cpu.cycle_count == expected_cycle_count);
  assert(!cpu.halted);
}

static void test_cpu_step_reports_stack_errors(void)
{
  const uint8_t value = UINT8_C(0xA5);
  const uint8_t sentinel = UINT8_C(0x3C);
  const uint64_t expected_cycle_count = UINT64_C(1);
  const uint8_t push_program[] = {OPCODE_PUSH_A};
  const uint8_t pop_program[] = {OPCODE_POP_A};
  const uint8_t expected_program_counter =
    (uint8_t)(
      sizeof push_program /
      sizeof push_program[0]
    );

  Cpu overflow_cpu = {
    .register_a = value,
    .stack_pointer = CPU_STACK_LOW_ADDRESS,
    .zero_flag = true,
    .carry_flag = true
  };

  const bool push_loaded = cpu_load_program(
    &overflow_cpu,
    push_program,
    sizeof push_program
  );

  assert(push_loaded);

  cpu_write_memory(
    &overflow_cpu,
    CPU_STACK_LOW_ADDRESS,
    sentinel
  );

  const CpuStepResult overflow_result =
    cpu_step(&overflow_cpu);

  assert(overflow_result == CPU_STEP_STACK_OVERFLOW);
  assert(overflow_cpu.halted);
  assert(
    overflow_cpu.stack_pointer ==
    CPU_STACK_LOW_ADDRESS
  );
  assert(
    cpu_read_memory(
      &overflow_cpu,
      CPU_STACK_LOW_ADDRESS
    ) == sentinel
  );
  assert(overflow_cpu.register_a == value);
  assert(overflow_cpu.zero_flag);
  assert(overflow_cpu.carry_flag);
  assert(
    overflow_cpu.program_counter ==
    expected_program_counter
  );
  assert(
    overflow_cpu.cycle_count ==
    expected_cycle_count
  );

  Cpu underflow_cpu = {
    .register_a = value,
    .zero_flag = true,
    .carry_flag = true
  };

  const bool pop_loaded = cpu_load_program(
    &underflow_cpu,
    pop_program,
    sizeof pop_program
  );

  assert(pop_loaded);

  const CpuStepResult underflow_result =
    cpu_step(&underflow_cpu);

  assert(underflow_result == CPU_STEP_STACK_UNDERFLOW);
  assert(underflow_cpu.halted);
  assert(
    underflow_cpu.stack_pointer ==
    CPU_STACK_EMPTY_POINTER
  );
  assert(underflow_cpu.register_a == value);
  assert(underflow_cpu.zero_flag);
  assert(underflow_cpu.carry_flag);
  assert(
    underflow_cpu.program_counter ==
    expected_program_counter
  );
  assert(
    underflow_cpu.cycle_count ==
    expected_cycle_count
  );
}

static void test_cpu_run_reports_stack_errors(void)
{
  const uint8_t push_program[] = {OPCODE_PUSH_A};
  const uint8_t pop_program[] = {OPCODE_POP_A};
  const uint64_t instruction_limit = UINT64_C(1);

  Cpu overflow_cpu = {
    .stack_pointer = CPU_STACK_LOW_ADDRESS
  };

  const bool push_loaded = cpu_load_program(
    &overflow_cpu,
    push_program,
    sizeof push_program
  );

  assert(push_loaded);

  const CpuRunResult overflow_result = cpu_run(
    &overflow_cpu,
    instruction_limit
  );

  assert(overflow_result == CPU_RUN_STACK_OVERFLOW);

  Cpu underflow_cpu = {0};

  const bool pop_loaded = cpu_load_program(
    &underflow_cpu,
    pop_program,
    sizeof pop_program
  );

  assert(pop_loaded);

  const CpuRunResult underflow_result = cpu_run(
    &underflow_cpu,
    instruction_limit
  );

  assert(underflow_result == CPU_RUN_STACK_UNDERFLOW);
}

int main(void)
{
  test_cpu_reset_clears_state();
  test_cpu_memory_read_and_write();
  test_cpu_fetch_byte_reads_and_advances_program_counter();
  test_cpu_step_executes_nop_and_halt();
  test_cpu_step_executes_load_immediate_a();
  test_cpu_step_rejects_invalid_opcode();
  test_cpu_load_program_copies_valid_program();
  test_cpu_run_stops_at_halt();
  test_cpu_run_reports_invalid_opcode();
  test_cpu_run_enforces_instruction_limit();
  test_cpu_run_handles_zero_limit_and_halted_cpu();
  test_cpu_load_program_accepts_full_program_region();
  test_cpu_load_program_validates_input();
  test_cpu_step_executes_load_immediate_b();
  test_cpu_step_executes_add_a_b();
  test_cpu_step_executes_sub_a_b();
  test_cpu_step_executes_bitwise_operations();
  test_cpu_step_executes_compare_a_b();
  test_cpu_step_executes_conditional_branches();
  test_cpu_step_executes_jump();
  test_cpu_step_executes_load_a_from_memory();
  test_cpu_step_executes_store_a_to_memory();
  test_cpu_step_executes_push_a();
  test_cpu_step_executes_pop_a();
  test_cpu_step_reports_stack_errors();
  test_cpu_run_reports_stack_errors();

  puts("All CPU tests passed.");

  return 0;
}
