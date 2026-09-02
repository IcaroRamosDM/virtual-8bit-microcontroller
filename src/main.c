#include <stdio.h>

#include "cpu.h"

int main(void)
{
  Cpu cpu = {
    .register_a = 128,
    .program_counter = 64,
    .halted = true
  };

  cpu.memory[0] = 255;

  cpu_reset(&cpu);

  puts("Virtual 8-bit microcontroller simulator");
  printf("Register A: %u\n", (unsigned int)cpu.register_a);
  printf("Program counter: %u\n", (unsigned int)cpu.program_counter);
  printf("Memory size: %zu bytes\n", sizeof cpu.memory);
  printf("Memory[0]: %u\n", (unsigned int)cpu.memory[0]);
  printf("Halted: %s\n", cpu.halted ? "true" : "false");

  return 0;
}

