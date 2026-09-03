#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "symbol_table.h"

enum
{
  TEST_GENERATED_NAME_BUFFER_SIZE = 32
};

static void test_initializes_empty_table(void)
{
  SymbolTable table;

  symbol_table_initialize(&table);

  assert(table.count == 0);
}

static void test_adds_and_finds_symbols(void)
{
  enum
  {
    START_ADDRESS = 0x00,
    MEMORY_DEMO_ADDRESS = 0x09
  };

  SymbolTable table;

  symbol_table_initialize(&table);

  const SymbolTableAddResult start_result =
    symbol_table_add(
      &table,
      "start",
      START_ADDRESS
    );

  const SymbolTableAddResult memory_demo_result =
    symbol_table_add(
      &table,
      "memory_demo",
      MEMORY_DEMO_ADDRESS
    );

  assert(start_result == SYMBOL_TABLE_ADD_SUCCESS);
  assert(memory_demo_result == SYMBOL_TABLE_ADD_SUCCESS);
  assert(table.count == 2);

  uint8_t address = 0;

  const bool start_was_found =
    symbol_table_find(&table, "start", &address);

  assert(start_was_found);
  assert(address == START_ADDRESS);

  const bool memory_demo_was_found =
    symbol_table_find(
      &table,
      "memory_demo",
      &address
    );

  assert(memory_demo_was_found);
  assert(address == MEMORY_DEMO_ADDRESS);
}

static void test_reports_missing_symbol(void)
{
  SymbolTable table;

  symbol_table_initialize(&table);

  uint8_t address = UINT8_C(0xA5);

  const bool symbol_was_found =
    symbol_table_find(
      &table,
      "missing",
      &address
    );

  assert(!symbol_was_found);
  assert(address == UINT8_C(0xA5));
}

static void test_rejects_duplicate_symbol(void)
{
  enum
  {
    ORIGINAL_ADDRESS = 0x10,
    DUPLICATE_ADDRESS = 0x20
  };

  SymbolTable table;

  symbol_table_initialize(&table);

  const SymbolTableAddResult original_result =
    symbol_table_add(
      &table,
      "loop",
      ORIGINAL_ADDRESS
    );

  const SymbolTableAddResult duplicate_result =
    symbol_table_add(
      &table,
      "loop",
      DUPLICATE_ADDRESS
    );

  assert(original_result == SYMBOL_TABLE_ADD_SUCCESS);
  assert(duplicate_result == SYMBOL_TABLE_ADD_DUPLICATE);
  assert(table.count == 1);

  uint8_t stored_address = 0;

  const bool symbol_was_found =
    symbol_table_find(
      &table,
      "loop",
      &stored_address
    );

  assert(symbol_was_found);
  assert(stored_address == ORIGINAL_ADDRESS);
}

static void test_rejects_invalid_names(void)
{
  SymbolTable table;

  symbol_table_initialize(&table);

  const SymbolTableAddResult empty_name_result =
    symbol_table_add(&table, "", 0);

  assert(
    empty_name_result ==
    SYMBOL_TABLE_ADD_EMPTY_NAME
  );

  char oversized_name[SYMBOL_TABLE_MAX_NAME_LENGTH + 2];

  memset(
    oversized_name,
    'x',
    SYMBOL_TABLE_MAX_NAME_LENGTH + 1
  );

  oversized_name[SYMBOL_TABLE_MAX_NAME_LENGTH + 1] =
    '\0';

  const SymbolTableAddResult oversized_name_result =
    symbol_table_add(
      &table,
      oversized_name,
      0
    );

  assert(
    oversized_name_result ==
    SYMBOL_TABLE_ADD_NAME_TOO_LONG
  );

  assert(table.count == 0);
}

static void test_rejects_symbol_when_full(void)
{
  SymbolTable table;

  symbol_table_initialize(&table);

  for (
    size_t index = 0;
    index < SYMBOL_TABLE_CAPACITY;
    ++index
  )
  {
    char name[TEST_GENERATED_NAME_BUFFER_SIZE];

    const int written_character_count = snprintf(
      name,
      sizeof name,
      "label_%zu",
      index
    );

    assert(written_character_count >= 0);
    assert(
      (size_t)written_character_count <
      sizeof name
    );

    const SymbolTableAddResult add_result =
      symbol_table_add(
        &table,
        name,
        (uint8_t)index
      );

    assert(add_result == SYMBOL_TABLE_ADD_SUCCESS);
  }

  const SymbolTableAddResult overflow_result =
    symbol_table_add(&table, "overflow", 0);

  assert(overflow_result == SYMBOL_TABLE_ADD_FULL);
  assert(table.count == SYMBOL_TABLE_CAPACITY);
}

int main(void)
{
  test_initializes_empty_table();
  test_adds_and_finds_symbols();
  test_reports_missing_symbol();
  test_rejects_duplicate_symbol();
  test_rejects_invalid_names();
  test_rejects_symbol_when_full();

  puts("All symbol-table tests passed.");

  return EXIT_SUCCESS;
}
