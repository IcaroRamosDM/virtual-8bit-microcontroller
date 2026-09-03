#pragma once

#include <stdint.h>

#include "symbol_table.h"

typedef enum ByteOperandResolveResult
{
  BYTE_OPERAND_RESOLVE_SUCCESS,
  BYTE_OPERAND_RESOLVE_EMPTY,
  BYTE_OPERAND_RESOLVE_INVALID,
  BYTE_OPERAND_RESOLVE_OUT_OF_RANGE,
  BYTE_OPERAND_RESOLVE_UNDEFINED_SYMBOL
} ByteOperandResolveResult;

ByteOperandResolveResult byte_operand_resolve(
    const char *text,
    const SymbolTable *symbols,
    uint8_t *value
);
