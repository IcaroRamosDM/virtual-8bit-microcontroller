#pragma once

#include <stdbool.h>
#include <stdint.h>

enum
{
  CPU_MEMORY_SIZE = 256
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

void cpu_reset(Cpu *cpu);
