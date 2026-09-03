#include "binary_reader.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static bool close_input_file(
    FILE *input_file,
    const char *input_path
)
{
  const int close_result = fclose(input_file);

  if (close_result == 0)
  {
    return true;
  }

  const int close_error = errno;

  fprintf(
    stderr,
    "%s: could not close binary input: %s\n",
    input_path,
    strerror(close_error)
  );

  return false;
}

bool binary_reader_read(
    const char *input_path,
    uint8_t *bytes,
    size_t byte_capacity,
    size_t *byte_count
)
{
  if ((input_path == NULL) ||
      (bytes == NULL) ||
      (byte_count == NULL))
  {
    fputs("binary reader: invalid argument\n", stderr);
    return false;
  }

  *byte_count = 0;

  FILE *const input_file = fopen(input_path, "rb");

  if (input_file == NULL)
  {
    const int open_error = errno;

    fprintf(
      stderr,
      "%s: could not open binary input: %s\n",
      input_path,
      strerror(open_error)
    );

    return false;
  }

  const size_t read_byte_count = fread(
    bytes,
    sizeof bytes[0],
    byte_capacity,
    input_file
  );

  const int trailing_byte = fgetc(input_file);

  if (ferror(input_file) != 0)
  {
    const int read_error = errno;

    fprintf(
      stderr,
      "%s: could not read binary input: %s\n",
      input_path,
      strerror(read_error)
    );

    (void)close_input_file(input_file, input_path);

    return false;
  }

  if (trailing_byte != EOF)
  {
    fprintf(
      stderr,
      "%s: binary input exceeds %zu-byte capacity\n",
      input_path,
      byte_capacity
    );

    (void)close_input_file(input_file, input_path);

    return false;
  }

  if (!close_input_file(input_file, input_path))
  {
    return false;
  }

  *byte_count = read_byte_count;

  return true;
}
