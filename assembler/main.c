#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum
{
  ASSEMBLER_EXECUTABLE_PATH_INDEX = 0,
  ASSEMBLER_INPUT_PATH_INDEX = 1,
  ASSEMBLER_OUTPUT_PATH_INDEX = 2,
  ASSEMBLER_ARGUMENT_COUNT = 3,
  ASSEMBLER_MAX_SOURCE_LINE_LENGTH = 255,
  ASSEMBLER_LINE_BUFFER_SIZE =
    ASSEMBLER_MAX_SOURCE_LINE_LENGTH + 2
};

static bool source_line_is_too_long(
    FILE *source,
    const char *line
)
{
  const size_t stored_length = strlen(line);
  const bool has_newline =
    (stored_length > 0) &&
    (line[stored_length - 1] == '\n');
  const size_t content_length =
    has_newline ? stored_length - 1 : stored_length;

  return
    (content_length > ASSEMBLER_MAX_SOURCE_LINE_LENGTH) ||
    (!has_newline && !feof(source));
}

static bool read_source_file(
    const char *input_path,
    size_t *line_count
)
{
  FILE *source = fopen(input_path, "r");

  if (source == NULL)
  {
    const int error_number = errno;

    fprintf(
        stderr,
        "vm8asm: cannot open '%s': %s\n",
        input_path,
        strerror(error_number)
    );

    return false;
  }

  char line[ASSEMBLER_LINE_BUFFER_SIZE];

  *line_count = 0;

  while (fgets(line, sizeof line, source) != NULL)
  {
    ++(*line_count);

    if (source_line_is_too_long(source, line))
    {
      fprintf(
          stderr,
          "%s:%zu: source line exceeds %d characters\n",
          input_path,
          *line_count,
          ASSEMBLER_MAX_SOURCE_LINE_LENGTH
      );

      (void)fclose(source);
      return false;
    }
  }

  if (ferror(source))
  {
    fprintf(
        stderr,
        "vm8asm: failed to read '%s'.\n",
        input_path
    );

    (void)fclose(source);
    return false;
  }

  if (fclose(source) != 0)
  {
    fprintf(
        stderr,
        "vm8asm: failed to close '%s'.\n",
        input_path
    );

    return false;
  }

  return true;
}

int main(int argument_count, char *arguments[])
{
  if (argument_count != ASSEMBLER_ARGUMENT_COUNT)
  {
    fprintf(
        stderr,
        "Usage: %s <input.asm> <output.bin>\n",
        arguments[ASSEMBLER_EXECUTABLE_PATH_INDEX]
    );

    return EXIT_FAILURE;
  }

  const char *input_path =
    arguments[ASSEMBLER_INPUT_PATH_INDEX];
  const char *output_path =
    arguments[ASSEMBLER_OUTPUT_PATH_INDEX];
    size_t line_count = 0;

  if (!read_source_file(input_path, &line_count))
  {
    return EXIT_FAILURE;
  }

  printf(
      "Source read successfully: %zu line(s).\n",
      line_count
  );
  printf(
      "Output generation is not implemented yet: %s\n",
      output_path
  );

  return EXIT_SUCCESS;
}
