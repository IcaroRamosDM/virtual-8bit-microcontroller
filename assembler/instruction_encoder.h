#pragma once

#include <stddef.h>
#include <stdint.h>

#include "instruction_parser.h"
#include "symbol_table.h"

enum
{
  INSTRUCTION_MAX_ENCODED_SIZE = 2
};

typedef enum InstructionEncodeResult
{
  INSTRUCTION_ENCODE_SUCCESS,
  INSTRUCTION_ENCODE_UNKNOWN_MNEMONIC,
  INSTRUCTION_ENCODE_WRONG_OPERAND_COUNT,
  INSTRUCTION_ENCODE_INVALID_REGISTER,
  INSTRUCTION_ENCODE_EMPTY_OPERAND,
  INSTRUCTION_ENCODE_INVALID_OPERAND,
  INSTRUCTION_ENCODE_OPERAND_OUT_OF_RANGE,
  INSTRUCTION_ENCODE_UNDEFINED_SYMBOL
} InstructionEncodeResult;

typedef struct EncodedInstruction
{
  uint8_t bytes[INSTRUCTION_MAX_ENCODED_SIZE];
  size_t size;
} EncodedInstruction;

InstructionEncodeResult instruction_encode(
    const ParsedInstruction *instruction,
    const SymbolTable *symbols,
    EncodedInstruction *encoded_instruction
);
