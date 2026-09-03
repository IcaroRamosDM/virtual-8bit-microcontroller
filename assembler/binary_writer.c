#include "binary_writer.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static bool close_output_file(
    FILE *output_file,
    const char *output_path
)
{
  const int close_result = fclose(output_file);

  if (close_result == 0)
  {
    return true;
  }

  const int close_error = errno;

  fprintf(
    stderr,
    "%s: could not close output file: %s\n",
    output_path,
    strerror(close_error)
  );

  return false;
}

bool binary_writer_write(
    const char *output_path,
    const uint8_t *bytes,
    size_t byte_count
)
{
  FILE *const output_file =
    fopen(output_path, "wb");

  if (output_file == NULL)
  {
    const int open_error = errno;

    fprintf(
      stderr,
      "%s: could not open output file: %s\n",
      output_path,
      strerror(open_error)
    );

    return false;
  }

  const size_t written_byte_count =
    fwrite(
      bytes,
      sizeof bytes[0],
      byte_count,
      output_file
    );

  if (written_byte_count != byte_count)
  {
    fprintf(
      stderr,
      "%s: failed to write binary output: "
      "wrote %zu of %zu byte(s)\n",
      output_path,
      written_byte_count,
      byte_count
    );

    (void)close_output_file(
      output_file,
      output_path
    );

    return false;
  }

  return close_output_file(
    output_file,
    output_path
  );
}
