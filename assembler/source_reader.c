#include "source_reader.h"
#include "source_line.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

enum
{
  SOURCE_READER_LINE_BUFFER_SIZE =
    SOURCE_READER_MAX_LINE_LENGTH + 2
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
    (content_length > SOURCE_READER_MAX_LINE_LENGTH) ||
    (!has_newline && !feof(source));
}

bool source_reader_read(
    const char *input_path,
    SourceStatementHandler statement_handler,
    void *context,
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

  char line[SOURCE_READER_LINE_BUFFER_SIZE];

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
        SOURCE_READER_MAX_LINE_LENGTH
      );

      (void)fclose(source);
      return false;
    }

    source_line_normalize(line);

    if (
        (*line != '\0') &&
        !statement_handler(
          input_path,
          *line_count,
          line,
          context
        )
      )
    {
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
