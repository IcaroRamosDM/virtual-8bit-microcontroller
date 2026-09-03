#include "instruction_set.h"

#include <stddef.h>
#include <stdint.h>

static const InstructionMetadata INSTRUCTION_METADATA[] =
{
  {OPCODE_NOP, "NOP"},
  {OPCODE_HALT, "HALT"},
  {OPCODE_LOAD_IMMEDIATE_A, "LDI"},
  {OPCODE_LOAD_IMMEDIATE_B, "LDI"},
  {OPCODE_ADD_A_B, "ADD"},
  {OPCODE_SUB_A_B, "SUB"},
  {OPCODE_AND_A_B, "AND"},
  {OPCODE_OR_A_B, "OR"},
  {OPCODE_XOR_A_B, "XOR"},
  {OPCODE_NOT_A, "NOT"},
  {OPCODE_SHIFT_LEFT_A, "SHL"},
  {OPCODE_SHIFT_RIGHT_A, "SHR"},
  {OPCODE_COMPARE_A_B, "CMP"},
  {OPCODE_JUMP_IF_ZERO, "JZ"},
  {OPCODE_JUMP_IF_NOT_ZERO, "JNZ"},
  {OPCODE_JUMP_IF_CARRY, "JC"},
  {OPCODE_JUMP, "JMP"},
  {OPCODE_LOAD_A_FROM_MEMORY, "LDA"},
  {OPCODE_STORE_A_TO_MEMORY, "STA"}
};

const InstructionMetadata *instruction_set_find_by_opcode(
    uint8_t opcode
)
{
  const size_t instruction_count =
    sizeof INSTRUCTION_METADATA /
    sizeof INSTRUCTION_METADATA[0];

  for (
    size_t index = 0;
    index < instruction_count;
    ++index
  )
  {
    const InstructionMetadata *const metadata =
      &INSTRUCTION_METADATA[index];

    if ((uint8_t)metadata->opcode == opcode)
    {
      return metadata;
    }
  }

  return NULL;
}
