#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "instruction_set.h"

typedef struct InstructionExpectation
{
  Opcode opcode;
  const char *mnemonic;
} InstructionExpectation;

static void test_finds_every_instruction(void)
{
  static const InstructionExpectation expectations[] =
  {
    {OPCODE_NOP, "NOP"},
    {OPCODE_HALT, "HALT"},
    {OPCODE_LOAD_IMMEDIATE_A, "LDI"},
    {OPCODE_LOAD_IMMEDIATE_B, "LDI"},
    {OPCODE_ADD_A_B, "ADD"},
    {OPCODE_SUB_A_B, "SUB"},
    {OPCODE_JUMP_IF_ZERO, "JZ"},
    {OPCODE_JUMP, "JMP"},
    {OPCODE_LOAD_A_FROM_MEMORY, "LDA"},
    {OPCODE_STORE_A_TO_MEMORY, "STA"}
  };

  const size_t expectation_count =
    sizeof expectations / sizeof expectations[0];

  for (
    size_t index = 0;
    index < expectation_count;
    ++index
  )
  {
    const InstructionExpectation *const expectation =
      &expectations[index];

    const InstructionMetadata *const metadata =
      instruction_set_find_by_opcode(
        (uint8_t)expectation->opcode
      );

    assert(metadata != NULL);
    assert(metadata->opcode == expectation->opcode);
    assert(strcmp(metadata->mnemonic, expectation->mnemonic) == 0);
  }
}

static void test_rejects_unknown_opcode(void)
{
  const InstructionMetadata *const metadata =
    instruction_set_find_by_opcode(UINT8_MAX);

  assert(metadata == NULL);
}

int main(void)
{
  test_finds_every_instruction();
  test_rejects_unknown_opcode();

  puts("All instruction-set tests passed.");

  return EXIT_SUCCESS;
}
