#include "byte_literal.h"

#include <stdbool.h>
#include <stdint.h>

enum
{
  DECIMAL_BASE = 10,
  HEXADECIMAL_BASE = 16,
  HEXADECIMAL_FIRST_ALPHA_DIGIT_VALUE = 10
};

static bool character_to_digit(
    char character,
    unsigned int *digit
)
{
  if (
    (character >= '0') &&
    (character <= '9')
  )
  {
    *digit = (unsigned int)(character - '0');
    return true;
  }

  if (
    (character >= 'a') &&
    (character <= 'f')
  )
  {
    *digit =
      HEXADECIMAL_FIRST_ALPHA_DIGIT_VALUE +
      (unsigned int)(character - 'a');

    return true;
  }

  if (
    (character >= 'A') &&
    (character <= 'F')
  )
  {
    *digit =
      HEXADECIMAL_FIRST_ALPHA_DIGIT_VALUE +
      (unsigned int)(character - 'A');

    return true;
  }

  return false;
}

ByteLiteralParseResult byte_literal_parse(
    const char *text,
    uint8_t *value
)
{
  if (*text == '\0')
  {
    return BYTE_LITERAL_PARSE_EMPTY;
  }

  unsigned int base = DECIMAL_BASE;
  const char *digits = text;

  if (
    (text[0] == '0') &&
    (text[1] == 'x')
  )
  {
    base = HEXADECIMAL_BASE;
    digits += 2;
  }

  if (*digits == '\0')
  {
    return BYTE_LITERAL_PARSE_INVALID;
  }

  unsigned int accumulated_value = 0;

  for (
    const char *character = digits;
    *character != '\0';
    ++character
  )
  {
    unsigned int digit = 0;

    if (
      !character_to_digit(*character, &digit) ||
      (digit >= base)
    )
    {
      return BYTE_LITERAL_PARSE_INVALID;
    }

    if (
      accumulated_value >
      ((unsigned int)UINT8_MAX - digit) / base
    )
    {
      return BYTE_LITERAL_PARSE_OUT_OF_RANGE;
    }

    accumulated_value =
      (accumulated_value * base) + digit;
  }

  *value = (uint8_t)accumulated_value;

  return BYTE_LITERAL_PARSE_SUCCESS;
}
