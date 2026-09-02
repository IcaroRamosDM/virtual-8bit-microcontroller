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

    case OPCODE_HALT:
      cpu->halted = true;
      return CPU_STEP_HALTED;

    default:
      cpu->halted = true;
      return CPU_STEP_INVALID_OPCODE;
  }
}
