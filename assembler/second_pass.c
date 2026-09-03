#include "second_pass.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "instruction_encoder.h"
#include "instruction_parser.h"

static bool statement_is_label(const char *statement)
{
  const size_t statement_length = strlen(statement);

  return
    (statement_length > 0) &&
    (statement[statement_length - 1] == ':');
}

static const char *parse_result_message(
    InstructionParseResult result
)
{
  switch (result)
  {
    case INSTRUCTION_PARSE_SUCCESS:
      return "no parsing error";

    case INSTRUCTION_PARSE_EMPTY:
      return "empty instruction";

    case INSTRUCTION_PARSE_EXPECTED_MNEMONIC:
      return "expected instruction mnemonic";

    case INSTRUCTION_PARSE_EXPECTED_OPERAND:
      return "expected operand";

    case INSTRUCTION_PARSE_EXPECTED_COMMA:
      return "expected comma between operands";

    case INSTRUCTION_PARSE_TOO_MANY_OPERANDS:
      return "too many operands";

    case INSTRUCTION_PARSE_TOKEN_TOO_LONG:
      return "instruction token is too long";
  }

  return "unknown instruction syntax error";
}

static const char *encode_result_message(
    InstructionEncodeResult result
)
{
  switch (result)
  {
    case INSTRUCTION_ENCODE_SUCCESS:
      return "no encoding error";

    case INSTRUCTION_ENCODE_UNKNOWN_MNEMONIC:
      return "unknown instruction mnemonic";

    case INSTRUCTION_ENCODE_WRONG_OPERAND_COUNT:
      return "wrong number of operands";

    case INSTRUCTION_ENCODE_INVALID_REGISTER:
      return "invalid register or register order";

    case INSTRUCTION_ENCODE_EMPTY_OPERAND:
      return "empty operand";

    case INSTRUCTION_ENCODE_INVALID_OPERAND:
      return "invalid operand";

    case INSTRUCTION_ENCODE_OPERAND_OUT_OF_RANGE:
      return "operand is outside the 8-bit range";

    case INSTRUCTION_ENCODE_UNDEFINED_SYMBOL:
      return "undefined symbol";
  }

  return "unknown instruction encoding error";
}

void second_pass_initialize(
    SecondPassResult *result,
    const SymbolTable *symbols,
    uint8_t *program,
    size_t program_capacity
)
{
  result->symbols = symbols;
  result->program = program;
  result->program_capacity = program_capacity;
  result->program_size = 0;
  result->instruction_count = 0;
}

bool second_pass_process_statement(
    const char *input_path,
    size_t line_number,
    const char *statement,
    void *context
)
{
  SecondPassResult *result = context;

  if (statement_is_label(statement))
  {
    return true;
  }

  ParsedInstruction instruction;

  const InstructionParseResult parse_result =
    instruction_parse(
      statement,
      &instruction
    );

  if (parse_result != INSTRUCTION_PARSE_SUCCESS)
  {
    fprintf(
      stderr,
      "%s:%zu: %s: '%s'\n",
      input_path,
      line_number,
      parse_result_message(parse_result),
      statement
    );

    return false;
  }

  EncodedInstruction encoded_instruction = {0};

  const InstructionEncodeResult encode_result =
    instruction_encode(
      &instruction,
      result->symbols,
      &encoded_instruction
    );

  if (encode_result != INSTRUCTION_ENCODE_SUCCESS)
  {
    fprintf(
      stderr,
      "%s:%zu: %s: '%s'\n",
      input_path,
      line_number,
      encode_result_message(encode_result),
      statement
    );

    return false;
  }

  if (
    (result->program_size > result->program_capacity) ||
    (
      encoded_instruction.size >
      result->program_capacity - result->program_size
    )
  )
  {
    fprintf(
      stderr,
      "%s:%zu: encoded program exceeds "
      "%zu-byte output capacity\n",
      input_path,
      line_number,
      result->program_capacity
    );

    return false;
  }

  memcpy(
    result->program + result->program_size,
    encoded_instruction.bytes,
    encoded_instruction.size
  );

  result->program_size += encoded_instruction.size;
  ++result->instruction_count;

  return true;
}
