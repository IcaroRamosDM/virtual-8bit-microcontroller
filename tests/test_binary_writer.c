#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "binary_writer.h"

static const char TEST_OUTPUT_PATH[] =
  "build/test_binary_writer_output.bin";

static const uint8_t EXPECTED_BYTES[] =
{
  UINT8_C(0x10),
  UINT8_C(0x00),
  UINT8_C(0xFF),
  UINT8_C(0x01)
};

static void test_writes_exact_binary_bytes(void)
{
  const size_t expected_byte_count =
    sizeof EXPECTED_BYTES /
    sizeof EXPECTED_BYTES[0];

  const bool write_succeeded =
    binary_writer_write(
      TEST_OUTPUT_PATH,
      EXPECTED_BYTES,
      expected_byte_count
    );

  assert(write_succeeded);

  FILE *const input_file =
    fopen(TEST_OUTPUT_PATH, "rb");

  assert(input_file != NULL);

  uint8_t actual_bytes[
    sizeof EXPECTED_BYTES /
    sizeof EXPECTED_BYTES[0]
  ] = {0};

  const size_t read_byte_count =
    fread(
      actual_bytes,
      sizeof actual_bytes[0],
      expected_byte_count,
      input_file
    );

  assert(read_byte_count == expected_byte_count);

  for (
    size_t index = 0;
    index < expected_byte_count;
    ++index
  )
  {
    assert(
      actual_bytes[index] ==
      EXPECTED_BYTES[index]
    );
  }

  const int trailing_byte = fgetc(input_file);

  assert(trailing_byte == EOF);
  assert(feof(input_file) != 0);
  assert(ferror(input_file) == 0);

  const int close_result = fclose(input_file);

  assert(close_result == 0);

  const int remove_result =
    remove(TEST_OUTPUT_PATH);

  assert(remove_result == 0);
}

int main(void)
{
  test_writes_exact_binary_bytes();

  puts("All binary-writer tests passed.");

  return EXIT_SUCCESS;
}
