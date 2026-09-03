#include "instruction_encoder.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "byte_operand.h"
#include "instruction_set.h"

enum
{
  FIRST_OPERAND_INDEX = 0,
  SECOND_OPERAND_INDEX = 1,
  OPCODE_BYTE_INDEX = 0,
  OPERAND_BYTE_INDEX = 1,
  NO_OPERAND_COUNT = 0,
  ONE_OPERAND_COUNT = 1,
  TWO_OPERAND_COUNT = 2,
  ONE_BYTE_INSTRUCTION_SIZE = 1,
  TWO_BYTE_INSTRUCTION_SIZE = 2
};

static void encode_one_byte(
    EncodedInstruction *encoded_instruction,
    Opcode opcode
)
{
  encoded_instruction->bytes[OPCODE_BYTE_INDEX] =
    (uint8_t)opcode;

  encoded_instruction->bytes[OPERAND_BYTE_INDEX] = 0;
  encoded_instruction->size =
    ONE_BYTE_INSTRUCTION_SIZE;
}

static void encode_two_bytes(
    EncodedInstruction *encoded_instruction,
    Opcode opcode,
    uint8_t operand
)
{
  encoded_instruction->bytes[OPCODE_BYTE_INDEX] =
    (uint8_t)opcode;

  encoded_instruction->bytes[OPERAND_BYTE_INDEX] =
    operand;

  encoded_instruction->size =
    TWO_BYTE_INSTRUCTION_SIZE;
}

static InstructionEncodeResult map_operand_result(
    ByteOperandResolveResult result
)
{
  switch (result)
  {
    case BYTE_OPERAND_RESOLVE_SUCCESS:
      return INSTRUCTION_ENCODE_SUCCESS;

    case BYTE_OPERAND_RESOLVE_EMPTY:
      return INSTRUCTION_ENCODE_EMPTY_OPERAND;

    case BYTE_OPERAND_RESOLVE_INVALID:
      return INSTRUCTION_ENCODE_INVALID_OPERAND;

    case BYTE_OPERAND_RESOLVE_OUT_OF_RANGE:
      return INSTRUCTION_ENCODE_OPERAND_OUT_OF_RANGE;

    case BYTE_OPERAND_RESOLVE_UNDEFINED_SYMBOL:
      return INSTRUCTION_ENCODE_UNDEFINED_SYMBOL;
  }

  return INSTRUCTION_ENCODE_INVALID_OPERAND;
}

static InstructionEncodeResult encode_without_operands(
    const ParsedInstruction *instruction,
    Opcode opcode,
    EncodedInstruction *encoded_instruction
)
{
  if (instruction->operand_count != NO_OPERAND_COUNT)
  {
    return INSTRUCTION_ENCODE_WRONG_OPERAND_COUNT;
  }

  encode_one_byte(encoded_instruction, opcode);

  return INSTRUCTION_ENCODE_SUCCESS;
}

static InstructionEncodeResult encode_load_immediate(
    const ParsedInstruction *instruction,
    const SymbolTable *symbols,
    EncodedInstruction *encoded_instruction
)
{
  if (instruction->operand_count != TWO_OPERAND_COUNT)
  {
    return INSTRUCTION_ENCODE_WRONG_OPERAND_COUNT;
  }

  Opcode opcode;

  if (
    strcmp(
      instruction->operands[FIRST_OPERAND_INDEX],
      "A"
    ) == 0
  )
  {
    opcode = OPCODE_LOAD_IMMEDIATE_A;
  }
  else if (
    strcmp(
      instruction->operands[FIRST_OPERAND_INDEX],
      "B"
    ) == 0
  )
  {
    opcode = OPCODE_LOAD_IMMEDIATE_B;
  }
  else
  {
    return INSTRUCTION_ENCODE_INVALID_REGISTER;
  }

  uint8_t value = 0;

  const ByteOperandResolveResult operand_result =
    byte_operand_resolve(
      instruction->operands[SECOND_OPERAND_INDEX],
      symbols,
      &value
    );

  if (operand_result != BYTE_OPERAND_RESOLVE_SUCCESS)
  {
    return map_operand_result(operand_result);
  }

  encode_two_bytes(
    encoded_instruction,
    opcode,
    value
  );

  return INSTRUCTION_ENCODE_SUCCESS;
}

static InstructionEncodeResult encode_register_pair(
    const ParsedInstruction *instruction,
    Opcode opcode,
    EncodedInstruction *encoded_instruction
)
{
  if (instruction->operand_count != TWO_OPERAND_COUNT)
  {
    return INSTRUCTION_ENCODE_WRONG_OPERAND_COUNT;
  }

  if (
    strcmp(
      instruction->operands[FIRST_OPERAND_INDEX],
      "A"
    ) != 0 ||
    strcmp(
      instruction->operands[SECOND_OPERAND_INDEX],
      "B"
    ) != 0
  )
  {
    return INSTRUCTION_ENCODE_INVALID_REGISTER;
  }

  encode_one_byte(encoded_instruction, opcode);

  return INSTRUCTION_ENCODE_SUCCESS;
}

static InstructionEncodeResult
encode_byte_operand_instruction(
    const ParsedInstruction *instruction,
    const SymbolTable *symbols,
    Opcode opcode,
    EncodedInstruction *encoded_instruction
)
{
  if (instruction->operand_count != ONE_OPERAND_COUNT)
  {
    return INSTRUCTION_ENCODE_WRONG_OPERAND_COUNT;
  }

  uint8_t value = 0;

  const ByteOperandResolveResult operand_result =
    byte_operand_resolve(
      instruction->operands[FIRST_OPERAND_INDEX],
      symbols,
      &value
    );

  if (operand_result != BYTE_OPERAND_RESOLVE_SUCCESS)
  {
    return map_operand_result(operand_result);
  }

  encode_two_bytes(
    encoded_instruction,
    opcode,
    value
  );

  return INSTRUCTION_ENCODE_SUCCESS;
}

InstructionEncodeResult instruction_encode(
    const ParsedInstruction *instruction,
    const SymbolTable *symbols,
    EncodedInstruction *encoded_instruction
)
{
  EncodedInstruction candidate = {0};

  InstructionEncodeResult result =
    INSTRUCTION_ENCODE_UNKNOWN_MNEMONIC;

  if (strcmp(instruction->mnemonic, "NOP") == 0)
  {
    result = encode_without_operands(
      instruction,
      OPCODE_NOP,
      &candidate
    );
  }
  else if (strcmp(instruction->mnemonic, "HALT") == 0)
  {
    result = encode_without_operands(
      instruction,
      OPCODE_HALT,
      &candidate
    );
  }
  else if (strcmp(instruction->mnemonic, "LDI") == 0)
  {
    result = encode_load_immediate(
      instruction,
      symbols,
      &candidate
    );
  }
  else if (strcmp(instruction->mnemonic, "ADD") == 0)
  {
    result = encode_register_pair(
      instruction,
      OPCODE_ADD_A_B,
      &candidate
    );
  }
  else if (strcmp(instruction->mnemonic, "SUB") == 0)
  {
    result = encode_register_pair(
      instruction,
      OPCODE_SUB_A_B,
      &candidate
    );
  }
  else if (strcmp(instruction->mnemonic, "JZ") == 0)
  {
    result = encode_byte_operand_instruction(
      instruction,
      symbols,
      OPCODE_JUMP_IF_ZERO,
      &candidate
    );
  }
  else if (strcmp(instruction->mnemonic, "JMP") == 0)
  {
    result = encode_byte_operand_instruction(
      instruction,
      symbols,
      OPCODE_JUMP,
      &candidate
    );
  }
  else if (strcmp(instruction->mnemonic, "LDA") == 0)
  {
    result = encode_byte_operand_instruction(
      instruction,
      symbols,
      OPCODE_LOAD_A_FROM_MEMORY,
      &candidate
    );
  }
  else if (strcmp(instruction->mnemonic, "STA") == 0)
  {
    result = encode_byte_operand_instruction(
      instruction,
      symbols,
      OPCODE_STORE_A_TO_MEMORY,
      &candidate
    );
  }

  if (result == INSTRUCTION_ENCODE_SUCCESS)
  {
    *encoded_instruction = candidate;
  }

  return result;
}
