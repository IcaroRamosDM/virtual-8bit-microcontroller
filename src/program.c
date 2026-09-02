#include "program.h"

#include "cpu.h"

enum
{
  DEMO_EQUAL_OPERAND = 0x2A,
  DEMO_SKIPPED_A_VALUE = UINT8_MAX,
  DEMO_HALT_ADDRESS = 0x09
};

static const uint8_t demo_program_bytes[] = {
  OPCODE_LOAD_IMMEDIATE_A,
  DEMO_EQUAL_OPERAND,
  OPCODE_LOAD_IMMEDIATE_B,
  DEMO_EQUAL_OPERAND,
  OPCODE_SUB_A_B,
  OPCODE_JUMP_IF_ZERO,
  DEMO_HALT_ADDRESS,
  OPCODE_LOAD_IMMEDIATE_A,
  DEMO_SKIPPED_A_VALUE,
  OPCODE_HALT
};

Program program_get_demo(void)
{
  return (Program){
    .bytes = demo_program_bytes,
    .size =
      sizeof demo_program_bytes / sizeof demo_program_bytes[0]
  };
}
