#pragma once

#include <stdbool.h>
#include <stddef.h>

enum
{
  SOURCE_READER_MAX_LINE_LENGTH = 255
};

typedef bool (*SourceStatementHandler)(
    const char *input_path,
    size_t line_number,
    const char *statement,
    void *context
);

bool source_reader_read(
    const char *input_path,
    SourceStatementHandler statement_handler,
    void *context,
    size_t *line_count
);
