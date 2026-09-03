#pragma once

#include <stdint.h>

typedef enum ByteLiteralParseResult
{
  BYTE_LITERAL_PARSE_SUCCESS,
  BYTE_LITERAL_PARSE_EMPTY,
  BYTE_LITERAL_PARSE_INVALID,
  BYTE_LITERAL_PARSE_OUT_OF_RANGE
} ByteLiteralParseResult;

ByteLiteralParseResult byte_literal_parse(
    const char *text,
    uint8_t *value
);
