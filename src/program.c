#include "program.h"

#include "cpu.h"

enum
{
  DEMO_CLEARED_A_VALUE = 0,
  DEMO_EQUAL_OPERAND = 0x2A,
  DEMO_MEMORY_VALUE = 0x5A,
  DEMO_SKIPPED_A_VALUE = UINT8_MAX,
  DEMO_BRANCH_TARGET_ADDRESS = 0x09,
  DEMO_DATA_ADDRESS = 0x80
};

static const uint8_t demo_program_bytes[] = {
  OPCODE_LOAD_IMMEDIATE_A,
  DEMO_EQUAL_OPERAND,
  OPCODE_LOAD_IMMEDIATE_B,
  DEMO_EQUAL_OPERAND,
  OPCODE_SUB_A_B,
  OPCODE_JUMP_IF_ZERO,
  DEMO_BRANCH_TARGET_ADDRESS,
  OPCODE_LOAD_IMMEDIATE_A,
  DEMO_SKIPPED_A_VALUE,
  OPCODE_LOAD_IMMEDIATE_A,
  DEMO_MEMORY_VALUE,
  OPCODE_STORE_A_TO_MEMORY,
  DEMO_DATA_ADDRESS,
  OPCODE_LOAD_IMMEDIATE_A,
  DEMO_CLEARED_A_VALUE,
  OPCODE_LOAD_A_FROM_MEMORY,
  DEMO_DATA_ADDRESS,
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
