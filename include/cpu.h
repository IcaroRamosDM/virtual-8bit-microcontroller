#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum
{
  CPU_MEMORY_SIZE = UINT8_MAX + 1
};

typedef struct Cpu
{
  uint8_t register_a;
  uint8_t register_b;
  uint8_t program_counter;
  bool zero_flag;
  bool carry_flag;
  bool halted;
  uint64_t cycle_count;
  uint8_t memory[CPU_MEMORY_SIZE];
} Cpu;

typedef enum Opcode
{
  OPCODE_NOP = 0x00,
  OPCODE_HALT = 0x01,
  OPCODE_LOAD_IMMEDIATE_A = 0x10
} Opcode;

typedef enum CpuStepResult
{
  CPU_STEP_OK,
  CPU_STEP_HALTED,
  CPU_STEP_INVALID_OPCODE
} CpuStepResult;

typedef enum CpuRunResult
{
  CPU_RUN_HALTED,
  CPU_RUN_INVALID_OPCODE,
  CPU_RUN_INSTRUCTION_LIMIT_REACHED
} CpuRunResult;

void cpu_reset(Cpu *cpu);

bool cpu_load_program(
    Cpu *cpu,
    const uint8_t *program,
    size_t program_size
    );

uint8_t cpu_read_memory(const Cpu *cpu, uint8_t address);
void cpu_write_memory(Cpu *cpu, uint8_t address, uint8_t value);
uint8_t cpu_fetch_byte(Cpu *cpu);

CpuStepResult cpu_step(Cpu *cpu);
CpuRunResult cpu_run(Cpu *cpu, uint64_t instruction_limit);
