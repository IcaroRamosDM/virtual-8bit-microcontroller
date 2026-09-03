#include "first_pass.h"

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "cpu.h"

enum
{
  INSTRUCTION_SIZE_ONE_BYTE = 1,
  INSTRUCTION_SIZE_TWO_BYTES = 2
};

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
  {"JZ", INSTRUCTION_SIZE_TWO_BYTES},
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

static bool label_name_is_valid(const char *name)
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

static bool label_name_is_reserved(const char *name)
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

  if (!label_name_is_valid(label_name))
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

  if (label_name_is_reserved(label_name))
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

  return process_instruction(
    input_path,
    line_number,
    statement,
    result
  );
}
