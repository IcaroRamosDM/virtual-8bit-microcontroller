#include "cpu_trace.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include "instruction_set.h"

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

    case CPU_STEP_STACK_OVERFLOW:
      return "stack-overflow";

    case CPU_STEP_STACK_UNDERFLOW:
      return "stack-underflow";
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

  const InstructionMetadata *const metadata =
    instruction_set_find_by_opcode(opcode);

  const char *const mnemonic =
    (metadata == NULL)
      ? "UNKNOWN"
      : metadata->mnemonic;

  fprintf(
    output,
    "  ADDR=0x%02X OP=0x%02X "
    "MNEMONIC=%s "
    "A=0x%02X B=0x%02X SP=0x%02X "
    "Z=%u C=%u NEXT=0x%02X "
    "CYCLES=%" PRIu64 " RESULT=%s\n",
    (unsigned int)instruction_address,
    (unsigned int)opcode,
    mnemonic,
    (unsigned int)cpu->register_a,
    (unsigned int)cpu->register_b,
    (unsigned int)cpu->stack_pointer,
    cpu->zero_flag ? 1U : 0U,
    cpu->carry_flag ? 1U : 0U,
    (unsigned int)cpu->program_counter,
    cpu->cycle_count,
    cpu_trace_step_result_name(step_result)
  );
}
