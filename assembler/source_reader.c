#include "source_reader.h"
#include "source_line.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

typedef enum SourceLineResult
{
  SOURCE_LINE_READY,
  SOURCE_LINE_END,
  SOURCE_LINE_TOO_LONG,
  SOURCE_LINE_BINARY,
  SOURCE_LINE_READ_ERROR
} SourceLineResult;

static SourceLineResult read_source_line(FILE *source, char *line)
{
  size_t length = 0;
  int character;

  while ((character = fgetc(source)) != EOF && character != '\n')
  {
    if (character == '\0')
    {
      return SOURCE_LINE_BINARY;
    }
    // Reserve one extra byte for the CR in a CRLF line ending.
    if (length == SOURCE_READER_MAX_LINE_LENGTH + 1)
    {
      return SOURCE_LINE_TOO_LONG;
    }
    line[length++] = (char)character;
  }

  if (ferror(source))
  {
    return SOURCE_LINE_READ_ERROR;
  }
  if (character == EOF && length == 0)
  {
    return SOURCE_LINE_END;
  }
  if (length > 0 && line[length - 1] == '\r')
  {
    --length;
  }
  if (length > SOURCE_READER_MAX_LINE_LENGTH)
  {
    return SOURCE_LINE_TOO_LONG;
  }
  line[length] = '\0';
  return SOURCE_LINE_READY;
}

bool source_reader_read(const char *input_path,
                        SourceStatementHandler statement_handler,
                        void *context,
                        size_t *line_count)
{
  FILE *source = fopen(input_path, "rb");

  if (source == NULL)
  {
    const int error_number = errno;

    fprintf(stderr, "vm8asm: cannot open '%s': %s\n", input_path, strerror(error_number));

    return false;
  }

  char line[SOURCE_READER_MAX_LINE_LENGTH + 2];

  *line_count = 0;

  for (;;)
  {
    const SourceLineResult result = read_source_line(source, line);
    if (result == SOURCE_LINE_END)
    {
      break;
    }
    ++(*line_count);

    if (result == SOURCE_LINE_TOO_LONG)
    {
      fprintf(stderr,
              "%s:%zu: source line exceeds %d characters\n",
              input_path,
              *line_count,
              SOURCE_READER_MAX_LINE_LENGTH);

      (void)fclose(source);
      return false;
    }
    if (result == SOURCE_LINE_BINARY || result == SOURCE_LINE_READ_ERROR)
    {
      fprintf(stderr,
              "%s:%zu: %s\n",
              input_path,
              *line_count,
              result == SOURCE_LINE_BINARY ? "source contains a NUL byte"
                                           : "failed to read source");
      (void)fclose(source);
      return false;
    }

    source_line_normalize(line);

    if ((*line != '\0') && !statement_handler(input_path, *line_count, line, context))
    {
      (void)fclose(source);
      return false;
    }
  }

  if (fclose(source) != 0)
  {
    fprintf(stderr, "vm8asm: failed to close '%s'.\n", input_path);

    return false;
  }

  return true;
}
