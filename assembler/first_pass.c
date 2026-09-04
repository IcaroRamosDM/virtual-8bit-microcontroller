#include "first_pass.h"

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "byte_literal.h"
#include "cpu.h"
#include "instruction_parser.h"

enum
{
  INSTRUCTION_SIZE_ONE_BYTE = 1,
  INSTRUCTION_SIZE_TWO_BYTES = 2,
  EQU_DIRECTIVE_OPERAND_COUNT = 2,
  BYTE_DIRECTIVE_OPERAND_COUNT = 1,
  EQU_NAME_OPERAND_INDEX = 0,
  EQU_VALUE_OPERAND_INDEX = 1
};

static const char EQU_DIRECTIVE[] = ".EQU";
static const char BYTE_DIRECTIVE[] = ".BYTE";

typedef struct InstructionDefinition
{
  const char *mnemonic;
  size_t encoded_size;
} InstructionDefinition;

static const InstructionDefinition
  INSTRUCTION_DEFINITIONS[] =
{
  {"NOP", INSTRUCTION_SIZE_ONE_BYTE},
  {"HALT", INSTRUCTION_SIZE_ONE_BYTE},
  {"LDI", INSTRUCTION_SIZE_TWO_BYTES},
  {"ADD", INSTRUCTION_SIZE_ONE_BYTE},
  {"SUB", INSTRUCTION_SIZE_ONE_BYTE},
  {"AND", INSTRUCTION_SIZE_ONE_BYTE},
  {"OR", INSTRUCTION_SIZE_ONE_BYTE},
  {"XOR", INSTRUCTION_SIZE_ONE_BYTE},
  {"NOT", INSTRUCTION_SIZE_ONE_BYTE},
  {"SHL", INSTRUCTION_SIZE_ONE_BYTE},
  {"SHR", INSTRUCTION_SIZE_ONE_BYTE},
  {"CMP", INSTRUCTION_SIZE_ONE_BYTE},
  {"JZ", INSTRUCTION_SIZE_TWO_BYTES},
  {"JNZ", INSTRUCTION_SIZE_TWO_BYTES},
  {"JC", INSTRUCTION_SIZE_TWO_BYTES},
  {"JMP", INSTRUCTION_SIZE_TWO_BYTES},
  {"LDA", INSTRUCTION_SIZE_TWO_BYTES},
  {"STA", INSTRUCTION_SIZE_TWO_BYTES}
};

static bool statement_has_mnemonic(
    const char *statement,
    const char *mnemonic
)
{
  const size_t mnemonic_length = strlen(mnemonic);

  if (
    strncmp(
      statement,
      mnemonic,
      mnemonic_length
    ) != 0
  )
  {
    return false;
  }

  const char next_character =
    statement[mnemonic_length];

  return
    (next_character == '\0') ||
    isspace((unsigned char)next_character);
}

static bool find_instruction_size(
    const char *statement,
    size_t *instruction_size
)
{
  const size_t definition_count =
    sizeof INSTRUCTION_DEFINITIONS /
    sizeof INSTRUCTION_DEFINITIONS[0];

  for (
    size_t index = 0;
    index < definition_count;
    ++index
  )
  {
    const InstructionDefinition *definition =
      &INSTRUCTION_DEFINITIONS[index];

    if (
      statement_has_mnemonic(
        statement,
        definition->mnemonic
      )
    )
    {
      *instruction_size = definition->encoded_size;
      return true;
    }
  }

  return false;
}

static bool symbol_name_is_valid(const char *name)
{
  if (*name == '\0')
  {
    return false;
  }

  const unsigned char first_character =
    (unsigned char)*name;

  if (
    !isalpha(first_character) &&
    (*name != '_')
  )
  {
    return false;
  }

  for (
    const char *character = name + 1;
    *character != '\0';
    ++character
  )
  {
    const unsigned char current_character =
      (unsigned char)*character;

    if (
      !isalnum(current_character) &&
      (*character != '_')
    )
    {
      return false;
    }
  }

  return true;
}

static bool symbol_name_is_reserved(const char *name)
{
  const size_t definition_count =
    sizeof INSTRUCTION_DEFINITIONS /
    sizeof INSTRUCTION_DEFINITIONS[0];

  for (
    size_t index = 0;
    index < definition_count;
    ++index
  )
  {
    if (
      strcmp(
        name,
        INSTRUCTION_DEFINITIONS[index].mnemonic
      ) == 0
    )
    {
      return true;
    }
  }

  return
    (strcmp(name, "A") == 0) ||
    (strcmp(name, "B") == 0);
}

static bool statement_is_label(const char *statement)
{
  const size_t statement_length = strlen(statement);

  return
    (statement_length > 0) &&
    (statement[statement_length - 1] == ':');
}

static bool process_label(
    const char *input_path,
    size_t line_number,
    const char *statement,
    FirstPassResult *result
)
{
  const size_t statement_length = strlen(statement);
  const size_t label_length = statement_length - 1;

  if (label_length > SYMBOL_TABLE_MAX_NAME_LENGTH)
  {
    fprintf(
      stderr,
      "%s:%zu: label name is too long\n",
      input_path,
      line_number
    );

    return false;
  }

  char label_name[SYMBOL_TABLE_NAME_BUFFER_SIZE];

  memcpy(
    label_name,
    statement,
    label_length
  );

  label_name[label_length] = '\0';

  if (!symbol_name_is_valid(label_name))
  {
    fprintf(
      stderr,
      "%s:%zu: invalid label '%s'\n",
      input_path,
      line_number,
      label_name
    );

    return false;
  }

  if (symbol_name_is_reserved(label_name))
  {
    fprintf(
      stderr,
      "%s:%zu: label name '%s' is reserved\n",
      input_path,
      line_number,
      label_name
    );

    return false;
  }

  if (result->program_size >= CPU_MEMORY_SIZE)
  {
    fprintf(
      stderr,
      "%s:%zu: label '%s' is outside "
      "8-bit memory\n",
      input_path,
      line_number,
      label_name
    );

    return false;
  }

  const SymbolTableAddResult add_result =
    symbol_table_add(
      &result->symbols,
      label_name,
      (uint8_t)result->program_size
    );

  switch (add_result)
  {
    case SYMBOL_TABLE_ADD_SUCCESS:
      return true;

    case SYMBOL_TABLE_ADD_DUPLICATE:
      fprintf(
        stderr,
        "%s:%zu: duplicate label '%s'\n",
        input_path,
        line_number,
        label_name
      );
      return false;

    case SYMBOL_TABLE_ADD_FULL:
      fprintf(
        stderr,
        "%s:%zu: symbol table is full\n",
        input_path,
        line_number
      );
      return false;

    case SYMBOL_TABLE_ADD_EMPTY_NAME:
      fprintf(
        stderr,
        "%s:%zu: label name cannot be empty\n",
        input_path,
        line_number
      );
      return false;

    case SYMBOL_TABLE_ADD_NAME_TOO_LONG:
      fprintf(
        stderr,
        "%s:%zu: label name is too long\n",
        input_path,
        line_number
      );
      return false;
  }

  return false;
}

static bool parse_directive(
    const char *input_path,
    size_t line_number,
    const char *statement,
    const char *directive_name,
    size_t expected_operand_count,
    ParsedInstruction *directive
)
{
  const InstructionParseResult parse_result =
    instruction_parse(statement, directive);

  if (
    (parse_result == INSTRUCTION_PARSE_SUCCESS) &&
    (directive->operand_count == expected_operand_count)
  )
  {
    return true;
  }

  fprintf(
    stderr,
    "%s:%zu: invalid %s directive: '%s'\n",
    input_path,
    line_number,
    directive_name,
    statement
  );

  return false;
}

static bool process_equ_directive(
    const char *input_path,
    size_t line_number,
    const char *statement,
    FirstPassResult *result
)
{
  ParsedInstruction directive = {0};

  if (
    !parse_directive(
      input_path,
      line_number,
      statement,
      EQU_DIRECTIVE,
      EQU_DIRECTIVE_OPERAND_COUNT,
      &directive
    )
  )
  {
    return false;
  }

  const char *name =
    directive.operands[EQU_NAME_OPERAND_INDEX];

  if (
    !symbol_name_is_valid(name) ||
    symbol_name_is_reserved(name)
  )
  {
    fprintf(
      stderr,
      "%s:%zu: invalid or reserved constant name '%s'\n",
      input_path,
      line_number,
      name
    );

    return false;
  }

  uint8_t value = 0;

  const ByteLiteralParseResult literal_result =
    byte_literal_parse(
      directive.operands[EQU_VALUE_OPERAND_INDEX],
      &value
    );

  if (literal_result != BYTE_LITERAL_PARSE_SUCCESS)
  {
    fprintf(
      stderr,
      "%s:%zu: constant value must be "
      "an 8-bit literal: '%s'\n",
      input_path,
      line_number,
      directive.operands[EQU_VALUE_OPERAND_INDEX]
    );

    return false;
  }

  const SymbolTableAddResult add_result =
    symbol_table_add(&result->symbols, name, value);

  if (add_result != SYMBOL_TABLE_ADD_SUCCESS)
  {
    fprintf(
      stderr,
      "%s:%zu: could not define constant '%s'\n",
      input_path,
      line_number,
      name
    );

    return false;
  }

  return true;
}

static bool process_byte_directive(
    const char *input_path,
    size_t line_number,
    const char *statement,
    FirstPassResult *result
)
{
  ParsedInstruction directive = {0};

  if (
    !parse_directive(
      input_path,
      line_number,
      statement,
      BYTE_DIRECTIVE,
      BYTE_DIRECTIVE_OPERAND_COUNT,
      &directive
    )
  )
  {
    return false;
  }

  if (
    result->program_size + INSTRUCTION_SIZE_ONE_BYTE >
    CPU_MEMORY_SIZE
  )
  {
    fprintf(
      stderr,
      "%s:%zu: program exceeds "
      "%d-byte memory\n",
      input_path,
      line_number,
      CPU_MEMORY_SIZE
    );

    return false;
  }

  result->program_size += INSTRUCTION_SIZE_ONE_BYTE;

  return true;
}

static bool process_instruction(
    const char *input_path,
    size_t line_number,
    const char *statement,
    FirstPassResult *result
)
{
  size_t instruction_size = 0;

  if (
    !find_instruction_size(
      statement,
      &instruction_size
    )
  )
  {
    fprintf(
      stderr,
      "%s:%zu: unknown instruction '%s'\n",
      input_path,
      line_number,
      statement
    );

    return false;
  }

  if (
    result->program_size + instruction_size >
    CPU_MEMORY_SIZE
  )
  {
    fprintf(
      stderr,
      "%s:%zu: program exceeds "
      "%d-byte memory\n",
      input_path,
      line_number,
      CPU_MEMORY_SIZE
    );

    return false;
  }

  result->program_size += instruction_size;

  return true;
}

void first_pass_initialize(FirstPassResult *result)
{
  symbol_table_initialize(&result->symbols);

  result->statement_count = 0;
  result->program_size = 0;
}

bool first_pass_process_statement(
    const char *input_path,
    size_t line_number,
    const char *statement,
    void *context
)
{
  FirstPassResult *result = context;

  ++result->statement_count;

  if (statement_is_label(statement))
  {
    return process_label(
      input_path,
      line_number,
      statement,
      result
    );
  }

  if (statement_has_mnemonic(statement, EQU_DIRECTIVE))
  {
    return process_equ_directive(
      input_path,
      line_number,
      statement,
      result
    );
  }

  if (statement_has_mnemonic(statement, BYTE_DIRECTIVE))
  {
    return process_byte_directive(
      input_path,
      line_number,
      statement,
      result
    );
  }

  return process_instruction(
    input_path,
    line_number,
    statement,
    result
  );
}
