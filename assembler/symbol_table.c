#include "symbol_table.h"

#include <string.h>

void symbol_table_initialize(SymbolTable *table)
{
  table->count = 0;
}

bool symbol_table_find(
    const SymbolTable *table,
    const char *name,
    uint8_t *address
)
{
  for (size_t index = 0; index < table->count; ++index)
  {
    const Symbol *symbol = &table->symbols[index];

    if (strcmp(symbol->name, name) == 0)
    {
      if (address != NULL)
      {
        *address = symbol->address;
      }

      return true;
    }
  }

  return false;
}

SymbolTableAddResult symbol_table_add(
    SymbolTable *table,
    const char *name,
    uint8_t address
)
{
  if (*name == '\0')
  {
    return SYMBOL_TABLE_ADD_EMPTY_NAME;
  }

  const size_t name_length = strlen(name);

  if (name_length > SYMBOL_TABLE_MAX_NAME_LENGTH)
  {
    return SYMBOL_TABLE_ADD_NAME_TOO_LONG;
  }

  if (symbol_table_find(table, name, NULL))
  {
    return SYMBOL_TABLE_ADD_DUPLICATE;
  }

  if (table->count >= SYMBOL_TABLE_CAPACITY)
  {
    return SYMBOL_TABLE_ADD_FULL;
  }

  Symbol *symbol = &table->symbols[table->count];

  memcpy(
    symbol->name,
    name,
    name_length + 1
  );

  symbol->address = address;

  ++table->count;

  return SYMBOL_TABLE_ADD_SUCCESS;
}
