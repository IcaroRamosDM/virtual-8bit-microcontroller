#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "source_reader.h"

enum
{
  ASSEMBLER_EXECUTABLE_PATH_INDEX = 0,
  ASSEMBLER_INPUT_PATH_INDEX = 1,
  ASSEMBLER_OUTPUT_PATH_INDEX = 2,
  ASSEMBLER_ARGUMENT_COUNT = 3
};

typedef struct SourceSummary
{
  size_t statement_count;
} SourceSummary;

static bool count_source_statement(
    const char *input_path,
    size_t line_number,
    const char *statement,
    void *context
)
{
  (void)input_path;
  (void)line_number;
  (void)statement;

  SourceSummary *summary = context;

  ++summary->statement_count;

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
  SourceSummary source_summary = {0};
  size_t line_count = 0;

  if (
    !source_reader_read(
      input_path,
      count_source_statement,
      &source_summary,
      &line_count
    )
  )
  {
    return EXIT_FAILURE;
  }

  printf(
    "Source read successfully: %zu line(s), "
    "%zu statement(s).\n",
    line_count,
    source_summary.statement_count
  );
  printf(
    "Output generation is not implemented yet: %s\n",
    output_path
  );

  return EXIT_SUCCESS;
}
