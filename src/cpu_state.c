#include "cpu_state.h"

#include <inttypes.h>
#include <stdio.h>

void cpu_state_print(FILE *output, const Cpu *cpu)
{
  fprintf(
    output,
    "Register A: 0x%02X\n",
    (unsigned int)cpu->register_a
  );

  fprintf(
    output,
    "Register B: 0x%02X\n",
    (unsigned int)cpu->register_b
  );

  fprintf(
    output,
    "Stack pointer: 0x%02X\n",
    (unsigned int)cpu->stack_pointer
  );

  fprintf(
    output,
    "Input port: 0x%02X\n",
    (unsigned int)cpu_read_memory(
      cpu,
      CPU_INPUT_PORT_ADDRESS
    )
  );

  fprintf(
    output,
    "Output port: 0x%02X\n",
    (unsigned int)cpu_get_output_port(cpu)
  );

  fprintf(
    output,
    "Zero flag: %s\n",
    cpu->zero_flag ? "set" : "clear"
  );

  fprintf(
    output,
    "Carry flag: %s\n",
    cpu->carry_flag ? "set" : "clear"
  );

  fprintf(
    output,
    "Program counter: %u\n",
    (unsigned int)cpu->program_counter
  );

  fprintf(
    output,
    "Cycle count: %" PRIu64 "\n",
    cpu->cycle_count
  );
}
