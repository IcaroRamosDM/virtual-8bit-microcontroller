#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "first_pass.h"
#include "source_reader.h"

enum
{
  ASSEMBLER_EXECUTABLE_PATH_INDEX = 0,
  ASSEMBLER_INPUT_PATH_INDEX = 1,
  ASSEMBLER_OUTPUT_PATH_INDEX = 2,
  ASSEMBLER_ARGUMENT_COUNT = 3
};

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

  FirstPassResult first_pass_result;
  size_t line_count = 0;

  first_pass_initialize(&first_pass_result);

  if (
    !source_reader_read(
      input_path,
      first_pass_process_statement,
      &first_pass_result,
      &line_count
    )
  )
  {
    return EXIT_FAILURE;
  }

  printf(
    "First pass completed: %zu line(s), "
    "%zu statement(s), %zu byte(s), "
    "%zu symbol(s).\n",
    line_count,
    first_pass_result.statement_count,
    first_pass_result.program_size,
    first_pass_result.symbols.count
  );

  printf(
    "Output generation is not implemented yet: %s\n",
    output_path
  );

  return EXIT_SUCCESS;
}
