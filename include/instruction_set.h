#pragma once

#include <stdint.h>

typedef enum Opcode
{
  OPCODE_NOP = 0x00,
  OPCODE_HALT = 0x01,
  OPCODE_LOAD_IMMEDIATE_A = 0x10,
  OPCODE_LOAD_IMMEDIATE_B = 0x11,
  OPCODE_ADD_A_B = 0x20,
  OPCODE_SUB_A_B = 0x21,
  OPCODE_AND_A_B = 0x22,
  OPCODE_OR_A_B = 0x23,
  OPCODE_XOR_A_B = 0x24,
  OPCODE_NOT_A = 0x25,
  OPCODE_SHIFT_LEFT_A = 0x26,
  OPCODE_SHIFT_RIGHT_A = 0x27,
  OPCODE_JUMP_IF_ZERO = 0x30,
  OPCODE_JUMP = 0x31,
  OPCODE_LOAD_A_FROM_MEMORY = 0x40,
  OPCODE_STORE_A_TO_MEMORY = 0x41
} Opcode;

typedef struct InstructionMetadata
{
  Opcode opcode;
  const char *mnemonic;
} InstructionMetadata;

const InstructionMetadata *instruction_set_find_by_opcode(
    uint8_t opcode
);
