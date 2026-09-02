#include <stdio.h>

#include "cpu.h"

int main(void)
{
  Cpu cpu = {0};

  puts("Virtual 8-bit microcontroller simulator");
  printf("Memory size: %zu bytes\n", sizeof cpu.memory);

  return 0;
}

