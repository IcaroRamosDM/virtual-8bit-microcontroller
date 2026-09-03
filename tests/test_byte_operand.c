#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "byte_operand.h"
#include "symbol_table.h"

typedef struct ValidByteOperandTestCase
{
  const char *text;
  uint8_t expected_value;
} ValidByteOperandTestCase;

static void test_resolves_literals(void)
{
  static const ValidByteOperandTestCase test_cases[] =
  {
    {"42", UINT8_C(42)},
    {"0x80", UINT8_C(0x80)}
  };

  const size_t test_case_count =
    sizeof test_cases / sizeof test_cases[0];

  SymbolTable symbols;

  symbol_table_initialize(&symbols);

  for (
    size_t index = 0;
    index < test_case_count;
    ++index
  )
  {
    uint8_t value = 0;

    const ByteOperandResolveResult result =
      byte_operand_resolve(
        test_cases[index].text,
        &symbols,
        &value
      );

    assert(result == BYTE_OPERAND_RESOLVE_SUCCESS);
    assert(value == test_cases[index].expected_value);
  }
}

static void test_resolves_defined_symbols(void)
{
  enum
  {
    TARGET_ADDRESS = 0x09,
    LOOP_ADDRESS = 0x20
  };

  SymbolTable symbols;

  symbol_table_initialize(&symbols);

  const SymbolTableAddResult target_result =
    symbol_table_add(
      &symbols,
      "target",
      TARGET_ADDRESS
    );

  const SymbolTableAddResult loop_result =
    symbol_table_add(
      &symbols,
      "loop",
      LOOP_ADDRESS
    );

  assert(target_result == SYMBOL_TABLE_ADD_SUCCESS);
  assert(loop_result == SYMBOL_TABLE_ADD_SUCCESS);

  uint8_t value = 0;

  const ByteOperandResolveResult target_resolve_result =
    byte_operand_resolve(
      "target",
      &symbols,
      &value
    );

  assert(
    target_resolve_result ==
    BYTE_OPERAND_RESOLVE_SUCCESS
  );
  assert(value == TARGET_ADDRESS);

  const ByteOperandResolveResult loop_resolve_result =
    byte_operand_resolve(
      "loop",
      &symbols,
      &value
    );

  assert(
    loop_resolve_result ==
    BYTE_OPERAND_RESOLVE_SUCCESS
  );
  assert(value == LOOP_ADDRESS);
}

static void test_reports_empty_operand(void)
{
  SymbolTable symbols;

  symbol_table_initialize(&symbols);

  uint8_t value = UINT8_C(0xA5);

  const ByteOperandResolveResult result =
    byte_operand_resolve(
      "",
      &symbols,
      &value
    );

  assert(result == BYTE_OPERAND_RESOLVE_EMPTY);
  assert(value == UINT8_C(0xA5));
}

static void test_reports_undefined_symbol(void)
{
  SymbolTable symbols;

  symbol_table_initialize(&symbols);

  uint8_t value = UINT8_C(0xA5);

  const ByteOperandResolveResult result =
    byte_operand_resolve(
      "missing",
      &symbols,
      &value
    );

  assert(
    result ==
    BYTE_OPERAND_RESOLVE_UNDEFINED_SYMBOL
  );
  assert(value == UINT8_C(0xA5));
}

static void test_rejects_invalid_operands(void)
{
  static const char *const invalid_operands[] =
  {
    "12abc",
    "bad-name",
    "+1",
    "-1",
    "0X2A"
  };

  const size_t operand_count =
    sizeof invalid_operands /
    sizeof invalid_operands[0];

  SymbolTable symbols;

  symbol_table_initialize(&symbols);

  for (
    size_t index = 0;
    index < operand_count;
    ++index
  )
  {
    uint8_t value = UINT8_C(0xA5);

    const ByteOperandResolveResult result =
      byte_operand_resolve(
        invalid_operands[index],
        &symbols,
        &value
      );

    assert(result == BYTE_OPERAND_RESOLVE_INVALID);
    assert(value == UINT8_C(0xA5));
  }
}

static void test_rejects_out_of_range_literals(void)
{
  SymbolTable symbols;

  symbol_table_initialize(&symbols);

  uint8_t value = UINT8_C(0xA5);

  const ByteOperandResolveResult result =
    byte_operand_resolve(
      "256",
      &symbols,
      &value
    );

  assert(
    result ==
    BYTE_OPERAND_RESOLVE_OUT_OF_RANGE
  );
  assert(value == UINT8_C(0xA5));
}

int main(void)
{
  test_resolves_literals();
  test_resolves_defined_symbols();
  test_reports_empty_operand();
  test_reports_undefined_symbol();
  test_rejects_invalid_operands();
  test_rejects_out_of_range_literals();

  puts("All byte-operand tests passed.");

  return EXIT_SUCCESS;
}
