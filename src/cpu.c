#include "cpu.h"

void cpu_reset(Cpu *cpu)
{
  *cpu = (Cpu){0};
}
