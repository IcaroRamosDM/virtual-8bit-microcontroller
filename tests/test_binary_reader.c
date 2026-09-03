#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "binary_reader.h"

enum
{
  TEST_BUFFER_CAPACITY = 4
};

static const char TEST_INPUT_PATH[] =
  "build/test_binary_reader_input.bin";

static const uint8_t EXPECTED_BYTES[TEST_BUFFER_CAPACITY] =
{
  UINT8_C(0x10),
  UINT8_C(0x2A),
  UINT8_C(0x00),
  UINT8_C(0x01)
};

static void write_test_file(
    const char *path,
    const uint8_t *bytes,
    size_t byte_count
)
{
  FILE *const output_file = fopen(path, "wb");

  assert(output_file != NULL);

  const size_t written_byte_count = fwrite(
    bytes,
    sizeof bytes[0],
    byte_count,
    output_file
  );

  assert(written_byte_count == byte_count);

  const int close_result = fclose(output_file);

  assert(close_result == 0);
}

static void remove_test_file(const char *path)
{
  const int remove_result = remove(path);

  assert(remove_result == 0);
}

static void test_reads_exact_binary_bytes(void)
{
  write_test_file(
    TEST_INPUT_PATH,
    EXPECTED_BYTES,
    sizeof EXPECTED_BYTES
  );

  uint8_t actual_bytes[TEST_BUFFER_CAPACITY] = {0};
  size_t actual_byte_count = 0;

  const bool read_succeeded = binary_reader_read(
    TEST_INPUT_PATH,
    actual_bytes,
    sizeof actual_bytes,
    &actual_byte_count
  );

  assert(read_succeeded);
  assert(actual_byte_count == sizeof EXPECTED_BYTES);

  for (
    size_t index = 0;
    index < actual_byte_count;
    ++index
  )
  {
    assert(actual_bytes[index] == EXPECTED_BYTES[index]);
  }

  remove_test_file(TEST_INPUT_PATH);
}

static void test_accepts_empty_binary_file(void)
{
  write_test_file(
    TEST_INPUT_PATH,
    EXPECTED_BYTES,
    0
  );

  uint8_t actual_bytes[TEST_BUFFER_CAPACITY] = {0};
  size_t actual_byte_count = sizeof actual_bytes;

  const bool read_succeeded = binary_reader_read(
    TEST_INPUT_PATH,
    actual_bytes,
    sizeof actual_bytes,
    &actual_byte_count
  );

  assert(read_succeeded);
  assert(actual_byte_count == 0);

  remove_test_file(TEST_INPUT_PATH);
}

static void test_rejects_input_larger_than_capacity(void)
{
  const uint8_t oversized_bytes[
    TEST_BUFFER_CAPACITY + 1
  ] = {0};

  write_test_file(
    TEST_INPUT_PATH,
    oversized_bytes,
    sizeof oversized_bytes
  );

  uint8_t actual_bytes[TEST_BUFFER_CAPACITY] = {0};
  size_t actual_byte_count = sizeof actual_bytes;

  const bool read_succeeded = binary_reader_read(
    TEST_INPUT_PATH,
    actual_bytes,
    sizeof actual_bytes,
    &actual_byte_count
  );

  assert(!read_succeeded);
  assert(actual_byte_count == 0);

  remove_test_file(TEST_INPUT_PATH);
}

int main(void)
{
  test_reads_exact_binary_bytes();
  test_accepts_empty_binary_file();
  test_rejects_input_larger_than_capacity();

  puts("All binary-reader tests passed.");

  return EXIT_SUCCESS;
}
