#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "instruction_set.h"

enum
{
  CPU_MEMORY_SIZE = UINT8_MAX + 1,
  CPU_STACK_CAPACITY = 16,
  CPU_PROGRAM_MEMORY_SIZE =
    CPU_MEMORY_SIZE - CPU_STACK_CAPACITY,
  CPU_STACK_LOW_ADDRESS = CPU_PROGRAM_MEMORY_SIZE,
  CPU_STACK_HIGH_ADDRESS = CPU_MEMORY_SIZE - 1,
  CPU_STACK_EMPTY_POINTER = 0
};

typedef struct Cpu
{
  uint8_t register_a;
  uint8_t register_b;
  uint8_t program_counter;
  uint8_t stack_pointer;
  bool zero_flag;
  bool carry_flag;
  bool halted;
  uint64_t cycle_count;
  uint8_t memory[CPU_MEMORY_SIZE];
} Cpu;

typedef enum CpuStepResult
{
  CPU_STEP_OK,
  CPU_STEP_HALTED,
  CPU_STEP_INVALID_OPCODE,
  CPU_STEP_STACK_OVERFLOW,
  CPU_STEP_STACK_UNDERFLOW
} CpuStepResult;

typedef void (*CpuStepObserver)(
    uint8_t instruction_address,
    uint8_t opcode,
    const Cpu *cpu,
    CpuStepResult step_result,
    void *context
);

typedef enum CpuRunResult
{
  CPU_RUN_HALTED,
  CPU_RUN_INVALID_OPCODE,
  CPU_RUN_INSTRUCTION_LIMIT_REACHED,
  CPU_RUN_STACK_OVERFLOW,
  CPU_RUN_STACK_UNDERFLOW
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

CpuRunResult cpu_run_with_observer(
    Cpu *cpu,
    uint64_t instruction_limit,
    CpuStepObserver observer,
    void *observer_context
);

CpuRunResult cpu_run(Cpu *cpu, uint64_t instruction_limit);
