#pragma once

#include <stddef.h>

#include "source_reader.h"

enum
{
  INSTRUCTION_MAX_OPERAND_COUNT = 2,
  INSTRUCTION_TOKEN_BUFFER_SIZE =
    SOURCE_READER_MAX_LINE_LENGTH + 1
};

typedef enum InstructionParseResult
{
  INSTRUCTION_PARSE_SUCCESS,
  INSTRUCTION_PARSE_EMPTY,
  INSTRUCTION_PARSE_EXPECTED_MNEMONIC,
  INSTRUCTION_PARSE_EXPECTED_OPERAND,
  INSTRUCTION_PARSE_EXPECTED_COMMA,
  INSTRUCTION_PARSE_TOO_MANY_OPERANDS,
  INSTRUCTION_PARSE_TOKEN_TOO_LONG
} InstructionParseResult;

typedef struct ParsedInstruction
{
  char mnemonic[INSTRUCTION_TOKEN_BUFFER_SIZE];
  char operands
    [INSTRUCTION_MAX_OPERAND_COUNT]
    [INSTRUCTION_TOKEN_BUFFER_SIZE];
  size_t operand_count;
} ParsedInstruction;

InstructionParseResult instruction_parse(
    const char *statement,
    ParsedInstruction *instruction
);
