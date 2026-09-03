#include "cpu_trace.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

static const char *cpu_trace_step_result_name(
    CpuStepResult step_result
)
{
  switch (step_result)
  {
    case CPU_STEP_OK:
      return "ok";

    case CPU_STEP_HALTED:
      return "halted";

    case CPU_STEP_INVALID_OPCODE:
      return "invalid-opcode";
  }

  return "unknown";
}

void cpu_trace_print_header(FILE *output)
{
  fputs("Execution trace:\n", output);
}

void cpu_trace_observer(
    uint8_t instruction_address,
    uint8_t opcode,
    const Cpu *cpu,
    CpuStepResult step_result,
    void *context
)
{
  FILE *const output = context;

  fprintf(
    output,
    "  ADDR=0x%02X OP=0x%02X "
    "A=0x%02X B=0x%02X "
    "Z=%u C=%u NEXT=0x%02X "
    "CYCLES=%" PRIu64 " RESULT=%s\n",
    (unsigned int)instruction_address,
    (unsigned int)opcode,
    (unsigned int)cpu->register_a,
    (unsigned int)cpu->register_b,
    cpu->zero_flag ? 1U : 0U,
    cpu->carry_flag ? 1U : 0U,
    (unsigned int)cpu->program_counter,
    cpu->cycle_count,
    cpu_trace_step_result_name(step_result)
  );
}
