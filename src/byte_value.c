#include "byte_value.h"

#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

bool byte_value_parse(
    const char *text,
    uint8_t *value
)
{
  if ((text == NULL) || (value == NULL))
  {
    return false;
  }

  if (
    (text[0] == '\0') ||
    isspace((unsigned char)text[0]) ||
    (text[0] == '+') ||
    (text[0] == '-')
  )
  {
    return false;
  }

  int base = 10;
  const char *digits = text;

  if (
    (text[0] == '0') &&
    ((text[1] == 'x') || (text[1] == 'X'))
  )
  {
    base = 16;
    digits += 2;
  }

  if (
    (digits[0] == '\0') ||
    isspace((unsigned char)digits[0]) ||
    (digits[0] == '+') ||
    (digits[0] == '-')
  )
  {
    return false;
  }

  errno = 0;

  char *end = NULL;

  const unsigned long parsed_value =
    strtoul(digits, &end, base);

  if (
    (errno != 0) ||
    (end == digits) ||
    (end[0] != '\0') ||
    (parsed_value > UINT8_MAX)
  )
  {
    return false;
  }

  *value = (uint8_t)parsed_value;

  return true;
}
