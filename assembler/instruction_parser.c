#include "instruction_parser.h"

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

static const char *skip_whitespace(const char *text)
{
  while (isspace((unsigned char)*text))
  {
    ++text;
  }

  return text;
}

static size_t token_length(const char *text)
{
  size_t length = 0;

  while (
    (text[length] != '\0') &&
    (text[length] != ',') &&
    !isspace((unsigned char)text[length])
  )
  {
    ++length;
  }

  return length;
}

static bool copy_token(
    char *destination,
    size_t destination_size,
    const char *source,
    size_t source_length
)
{
  if (source_length >= destination_size)
  {
    return false;
  }

  memcpy(
    destination,
    source,
    source_length
  );

  destination[source_length] = '\0';

  return true;
}

InstructionParseResult instruction_parse(
    const char *statement,
    ParsedInstruction *instruction
)
{
  instruction->mnemonic[0] = '\0';
  instruction->operand_count = 0;

  for (
    size_t index = 0;
    index < INSTRUCTION_MAX_OPERAND_COUNT;
    ++index
  )
  {
    instruction->operands[index][0] = '\0';
  }

  const char *cursor = skip_whitespace(statement);

  if (*cursor == '\0')
  {
    return INSTRUCTION_PARSE_EMPTY;
  }

  const size_t mnemonic_length = token_length(cursor);

  if (mnemonic_length == 0)
  {
    return INSTRUCTION_PARSE_EXPECTED_MNEMONIC;
  }

  if (
    !copy_token(
      instruction->mnemonic,
      sizeof instruction->mnemonic,
      cursor,
      mnemonic_length
    )
  )
  {
    return INSTRUCTION_PARSE_TOKEN_TOO_LONG;
  }

  cursor += mnemonic_length;

  if (*cursor == ',')
  {
    return INSTRUCTION_PARSE_EXPECTED_OPERAND;
  }

  cursor = skip_whitespace(cursor);

  if (*cursor == '\0')
  {
    return INSTRUCTION_PARSE_SUCCESS;
  }

  const size_t first_operand_length =
    token_length(cursor);

  if (first_operand_length == 0)
  {
    return INSTRUCTION_PARSE_EXPECTED_OPERAND;
  }

  if (
    !copy_token(
      instruction->operands[0],
      sizeof instruction->operands[0],
      cursor,
      first_operand_length
    )
  )
  {
    return INSTRUCTION_PARSE_TOKEN_TOO_LONG;
  }

  instruction->operand_count = 1;
  cursor += first_operand_length;
  cursor = skip_whitespace(cursor);

  if (*cursor == '\0')
  {
    return INSTRUCTION_PARSE_SUCCESS;
  }

  if (*cursor != ',')
  {
    return INSTRUCTION_PARSE_EXPECTED_COMMA;
  }

  ++cursor;
  cursor = skip_whitespace(cursor);

  if (*cursor == '\0')
  {
    return INSTRUCTION_PARSE_EXPECTED_OPERAND;
  }

  const size_t second_operand_length =
    token_length(cursor);

  if (second_operand_length == 0)
  {
    return INSTRUCTION_PARSE_EXPECTED_OPERAND;
  }

  if (
    !copy_token(
      instruction->operands[1],
      sizeof instruction->operands[1],
      cursor,
      second_operand_length
    )
  )
  {
    return INSTRUCTION_PARSE_TOKEN_TOO_LONG;
  }

  instruction->operand_count = 2;
  cursor += second_operand_length;
  cursor = skip_whitespace(cursor);

  if (*cursor != '\0')
  {
    return INSTRUCTION_PARSE_TOO_MANY_OPERANDS;
  }

  return INSTRUCTION_PARSE_SUCCESS;
}
