#include "byte_operand.h"

#include <ctype.h>
#include <stdbool.h>

#include "byte_literal.h"

static bool text_is_symbol_name(const char *text)
{
  if (*text == '\0')
  {
    return false;
  }

  const unsigned char first_character =
    (unsigned char)*text;

  if (
    !isalpha(first_character) &&
    (*text != '_')
  )
  {
    return false;
  }

  for (
    const char *character = text + 1;
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

ByteOperandResolveResult byte_operand_resolve(
    const char *text,
    const SymbolTable *symbols,
    uint8_t *value
)
{
  const ByteLiteralParseResult literal_result =
    byte_literal_parse(text, value);

  switch (literal_result)
  {
    case BYTE_LITERAL_PARSE_SUCCESS:
      return BYTE_OPERAND_RESOLVE_SUCCESS;

    case BYTE_LITERAL_PARSE_EMPTY:
      return BYTE_OPERAND_RESOLVE_EMPTY;

    case BYTE_LITERAL_PARSE_OUT_OF_RANGE:
      return BYTE_OPERAND_RESOLVE_OUT_OF_RANGE;

    case BYTE_LITERAL_PARSE_INVALID:
      break;
  }

  if (!text_is_symbol_name(text))
  {
    return BYTE_OPERAND_RESOLVE_INVALID;
  }

  if (!symbol_table_find(symbols, text, value))
  {
    return BYTE_OPERAND_RESOLVE_UNDEFINED_SYMBOL;
  }

  return BYTE_OPERAND_RESOLVE_SUCCESS;
}
