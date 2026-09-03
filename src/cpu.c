#include "cpu.h"

void cpu_reset(Cpu *cpu)
{
  *cpu = (Cpu){0};
}

uint8_t cpu_read_memory(const Cpu *cpu, uint8_t address)
{
  return cpu->memory[address];
}

void cpu_write_memory(Cpu *cpu, uint8_t address, uint8_t value)
{
  cpu->memory[address] = value;
}

uint8_t cpu_fetch_byte(Cpu *cpu)
{
  const uint8_t address = cpu->program_counter;
  const uint8_t value = cpu_read_memory(cpu, address);

  ++cpu->program_counter;

  return value;
}

CpuStepResult cpu_step(Cpu *cpu)
{
  if (cpu->halted)
  {
    return CPU_STEP_HALTED;
  }

  const uint8_t opcode = cpu_fetch_byte(cpu);

  ++cpu->cycle_count;

  switch ((Opcode)opcode)
  {
    case OPCODE_NOP:
      return CPU_STEP_OK;

    case OPCODE_LOAD_IMMEDIATE_A:
      cpu->register_a = cpu_fetch_byte(cpu);
      cpu->zero_flag = (cpu->register_a == 0);
      return CPU_STEP_OK;

    case OPCODE_LOAD_IMMEDIATE_B:
      cpu->register_b = cpu_fetch_byte(cpu);
      cpu->zero_flag = (cpu->register_b == 0);
      return CPU_STEP_OK;

    case OPCODE_ADD_A_B:
    {
      const uint16_t sum =
        (uint16_t)cpu->register_a +
        (uint16_t)cpu->register_b;

      cpu->register_a = (uint8_t)sum;
      cpu->zero_flag = (cpu->register_a == 0);
      cpu->carry_flag = (sum > UINT8_MAX);

      return CPU_STEP_OK;
    }

    case OPCODE_SUB_A_B:
    {
      const uint8_t original_a = cpu->register_a;
      const bool borrow = original_a < cpu->register_b;
      const uint8_t difference =
        (uint8_t)(original_a - cpu->register_b);

      cpu->register_a = difference;
      cpu->zero_flag = (difference == 0);
      cpu->carry_flag = borrow;

      return CPU_STEP_OK;
    }

    case OPCODE_JUMP_IF_ZERO:
    {
      const uint8_t target_address = cpu_fetch_byte(cpu);

      if (cpu->zero_flag)
      {
        cpu->program_counter = target_address;
      }

      return CPU_STEP_OK;
    }

    case OPCODE_JUMP:
    {
      const uint8_t target_address = cpu_fetch_byte(cpu);

      cpu->program_counter = target_address;

      return CPU_STEP_OK;
    }

    case OPCODE_LOAD_A_FROM_MEMORY:
    {
      const uint8_t address = cpu_fetch_byte(cpu);

      cpu->register_a = cpu_read_memory(cpu, address);
      cpu->zero_flag = (cpu->register_a == 0);

      return CPU_STEP_OK;
    }

    case OPCODE_STORE_A_TO_MEMORY:
    {
      const uint8_t address = cpu_fetch_byte(cpu);

      cpu_write_memory(cpu, address, cpu->register_a);

      return CPU_STEP_OK;
    }

    case OPCODE_HALT:
      cpu->halted = true;
      return CPU_STEP_HALTED;

    default:
      cpu->halted = true;
      return CPU_STEP_INVALID_OPCODE;
  }
}

bool cpu_load_program(
    Cpu *cpu,
    const uint8_t *program,
    size_t program_size
)
{
  if (program_size > sizeof cpu->memory)
  {
    return false;
  }

  if ((program == NULL) && (program_size != 0))
  {
    return false;
  }

  for (size_t index = 0; index < program_size; ++index)
  {
    cpu->memory[index] = program[index];
  }

  return true;
}
CpuRunResult cpu_run_with_observer(
    Cpu *cpu,
    uint64_t instruction_limit,
    CpuStepObserver observer,
    void *observer_context
)
{
  if (cpu->halted)
  {
    return CPU_RUN_HALTED;
  }

  for (
    uint64_t executed_instructions = 0;
    executed_instructions < instruction_limit;
    ++executed_instructions
  )
  {
    const uint8_t instruction_address =
      cpu->program_counter;

    const uint8_t opcode = cpu_read_memory(
      cpu,
      instruction_address
    );

    const CpuStepResult step_result = cpu_step(cpu);

    if (observer != NULL)
    {
      observer(
        instruction_address,
        opcode,
        cpu,
        step_result,
        observer_context
      );
    }

    switch (step_result)
    {
      case CPU_STEP_OK:
        break;

      case CPU_STEP_HALTED:
        return CPU_RUN_HALTED;

      case CPU_STEP_INVALID_OPCODE:
        return CPU_RUN_INVALID_OPCODE;
    }
  }

  return CPU_RUN_INSTRUCTION_LIMIT_REACHED;
}

CpuRunResult cpu_run(
    Cpu *cpu,
    uint64_t instruction_limit
)
{
  return cpu_run_with_observer(
    cpu,
    instruction_limit,
    NULL,
    NULL
  );
}
