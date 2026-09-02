#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "cpu.h"

int main(void)
{
  const uint8_t program[] = {
    OPCODE_NOP,
    OPCODE_HALT
  };
  const size_t program_size = sizeof program / sizeof program[0];
  CpuStepResult result = CPU_STEP_OK;
  Cpu cpu = {0};

  if (program_size > (size_t)CPU_MEMORY_SIZE)
  {
    fputs("Program does not fit in memory.\n", stderr);
    return EXIT_FAILURE;
  }

  for (size_t index = 0; index < program_size; ++index)
  {
    cpu_write_memory(&cpu, (uint8_t)index, program[index]);
  }

  while (result == CPU_STEP_OK)
  {
    result = cpu_step(&cpu);
  }

  puts("Virtual 8-bit microcontroller simulator");

  if (result == CPU_STEP_INVALID_OPCODE)
  {
    puts("Execution result: invalid opcode");
    return EXIT_FAILURE;
  }

  puts("Execution result: halted");
  printf("Program counter: %" PRIu8 "\n", cpu.program_counter);
  printf("Cycle count: %" PRIu64 "\n", cpu.cycle_count);

  return EXIT_SUCCESS;
}
