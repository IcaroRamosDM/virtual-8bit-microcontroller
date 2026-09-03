#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "byte_literal.h"

typedef struct ValidByteLiteralTestCase
{
  const char *text;
  uint8_t expected_value;
} ValidByteLiteralTestCase;

static void test_accepts_valid_decimal_literals(void)
{
  static const ValidByteLiteralTestCase test_cases[] =
  {
    {"0", UINT8_C(0)},
    {"42", UINT8_C(42)},
    {"255", UINT8_C(255)},
    {"007", UINT8_C(7)}
  };

  const size_t test_case_count =
    sizeof test_cases / sizeof test_cases[0];

  for (
    size_t index = 0;
    index < test_case_count;
    ++index
  )
  {
    uint8_t value = 0;

    const ByteLiteralParseResult result =
      byte_literal_parse(
        test_cases[index].text,
        &value
      );

    assert(result == BYTE_LITERAL_PARSE_SUCCESS);
    assert(value == test_cases[index].expected_value);
  }
}

static void test_accepts_valid_hexadecimal_literals(void)
{
  static const ValidByteLiteralTestCase test_cases[] =
  {
    {"0x00", UINT8_C(0x00)},
    {"0x2A", UINT8_C(0x2A)},
    {"0xFF", UINT8_C(0xFF)},
    {"0xff", UINT8_C(0xFF)}
  };

  const size_t test_case_count =
    sizeof test_cases / sizeof test_cases[0];

  for (
    size_t index = 0;
    index < test_case_count;
    ++index
  )
  {
    uint8_t value = 0;

    const ByteLiteralParseResult result =
      byte_literal_parse(
        test_cases[index].text,
        &value
      );

    assert(result == BYTE_LITERAL_PARSE_SUCCESS);
    assert(value == test_cases[index].expected_value);
  }
}

static void test_reports_empty_literal(void)
{
  uint8_t value = UINT8_C(0xA5);

  const ByteLiteralParseResult result =
    byte_literal_parse("", &value);

  assert(result == BYTE_LITERAL_PARSE_EMPTY);
  assert(value == UINT8_C(0xA5));
}

static void test_rejects_invalid_literals(void)
{
  static const char *const invalid_literals[] =
  {
    "0x",
    "0X2A",
    "2A",
    "-1",
    "+1",
    " 42",
    "42 ",
    "0xGG",
    "12abc"
  };

  const size_t literal_count =
    sizeof invalid_literals /
    sizeof invalid_literals[0];

  for (
    size_t index = 0;
    index < literal_count;
    ++index
  )
  {
    uint8_t value = UINT8_C(0xA5);

    const ByteLiteralParseResult result =
      byte_literal_parse(
        invalid_literals[index],
        &value
      );

    assert(result == BYTE_LITERAL_PARSE_INVALID);
    assert(value == UINT8_C(0xA5));
  }
}

static void test_rejects_out_of_range_literals(void)
{
  static const char *const out_of_range_literals[] =
  {
    "256",
    "999",
    "0x100",
    "0xFFFF"
  };

  const size_t literal_count =
    sizeof out_of_range_literals /
    sizeof out_of_range_literals[0];

  for (
    size_t index = 0;
    index < literal_count;
    ++index
  )
  {
    uint8_t value = UINT8_C(0xA5);

    const ByteLiteralParseResult result =
      byte_literal_parse(
        out_of_range_literals[index],
        &value
      );

    assert(result == BYTE_LITERAL_PARSE_OUT_OF_RANGE);
    assert(value == UINT8_C(0xA5));
  }
}

int main(void)
{
  test_accepts_valid_decimal_literals();
  test_accepts_valid_hexadecimal_literals();
  test_reports_empty_literal();
  test_rejects_invalid_literals();
  test_rejects_out_of_range_literals();

  puts("All byte-literal tests passed.");

  return EXIT_SUCCESS;
}
