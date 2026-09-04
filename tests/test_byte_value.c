#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "byte_value.h"

enum
{
  TEST_BYTE_VALUE = 0xA5,
  TEST_SENTINEL_VALUE = 0x5A
};

static void assert_rejected(const char *text)
{
  uint8_t value = TEST_SENTINEL_VALUE;

  const bool parsed = byte_value_parse(text, &value);

  assert(!parsed);
  assert(value == TEST_SENTINEL_VALUE);
}

static void test_accepts_decimal_and_hexadecimal_bytes(void)
{
  uint8_t decimal_value = 0;
  uint8_t hexadecimal_value = 0;
  uint8_t uppercase_hexadecimal_value = 0;
  uint8_t maximum_value = 0;

  const bool decimal_parsed = byte_value_parse(
    "165",
    &decimal_value
  );

  const bool hexadecimal_parsed = byte_value_parse(
    "0xA5",
    &hexadecimal_value
  );

  const bool uppercase_hexadecimal_parsed =
    byte_value_parse(
      "0XA5",
      &uppercase_hexadecimal_value
    );

  const bool maximum_parsed = byte_value_parse(
    "255",
    &maximum_value
  );

  assert(decimal_parsed);
  assert(hexadecimal_parsed);
  assert(uppercase_hexadecimal_parsed);
  assert(maximum_parsed);
  assert(decimal_value == TEST_BYTE_VALUE);
  assert(hexadecimal_value == TEST_BYTE_VALUE);
  assert(uppercase_hexadecimal_value == TEST_BYTE_VALUE);
  assert(maximum_value == UINT8_MAX);
}

static void test_rejects_invalid_bytes(void)
{
  assert_rejected(NULL);
  assert_rejected("");
  assert_rejected(" 1");
  assert_rejected("+1");
  assert_rejected("-1");
  assert_rejected("256");
  assert_rejected("0x");
  assert_rejected("0x100");
  assert_rejected("12abc");

  const bool null_output_parsed =
    byte_value_parse("0", NULL);

  assert(!null_output_parsed);
}

int main(void)
{
  test_accepts_decimal_and_hexadecimal_bytes();
  test_rejects_invalid_bytes();

  puts("All byte-value tests passed.");

  return EXIT_SUCCESS;
}
