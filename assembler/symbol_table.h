#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum
{
  SYMBOL_TABLE_CAPACITY = 256,
  SYMBOL_TABLE_MAX_NAME_LENGTH = 255,
  SYMBOL_TABLE_NAME_BUFFER_SIZE =
    SYMBOL_TABLE_MAX_NAME_LENGTH + 1
};

typedef enum SymbolTableAddResult
{
  SYMBOL_TABLE_ADD_SUCCESS,
  SYMBOL_TABLE_ADD_EMPTY_NAME,
  SYMBOL_TABLE_ADD_NAME_TOO_LONG,
  SYMBOL_TABLE_ADD_DUPLICATE,
  SYMBOL_TABLE_ADD_FULL
} SymbolTableAddResult;

typedef struct Symbol
{
  char name[SYMBOL_TABLE_NAME_BUFFER_SIZE];
  uint8_t address;
} Symbol;

typedef struct SymbolTable
{
  Symbol symbols[SYMBOL_TABLE_CAPACITY];
  size_t count;
} SymbolTable;

void symbol_table_initialize(SymbolTable *table);

SymbolTableAddResult symbol_table_add(
    SymbolTable *table,
    const char *name,
    uint8_t address
);

bool symbol_table_find(
    const SymbolTable *table,
    const char *name,
    uint8_t *address
);
