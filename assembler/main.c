#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "binary_writer.h"
#include "cpu.h"
#include "first_pass.h"
#include "second_pass.h"
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
  size_t first_pass_line_count = 0;

  first_pass_initialize(&first_pass_result);

  const bool first_pass_succeeded =
    source_reader_read(
      input_path,
      first_pass_process_statement,
      &first_pass_result,
      &first_pass_line_count
    );

  if (!first_pass_succeeded)
  {
    return EXIT_FAILURE;
  }

  printf(
    "First pass completed: %zu line(s), "
    "%zu statement(s), %zu byte(s), "
    "%zu symbol(s).\n",
    first_pass_line_count,
    first_pass_result.statement_count,
    first_pass_result.program_size,
    first_pass_result.symbols.count
  );

  uint8_t program[CPU_PROGRAM_MEMORY_SIZE] = {0};
  SecondPassResult second_pass_result;
  size_t second_pass_line_count = 0;

  second_pass_initialize(
    &second_pass_result,
    &first_pass_result.symbols,
    program,
    sizeof program
  );

  const bool second_pass_succeeded =
    source_reader_read(
      input_path,
      second_pass_process_statement,
      &second_pass_result,
      &second_pass_line_count
    );

  if (!second_pass_succeeded)
  {
    return EXIT_FAILURE;
  }

  if (second_pass_line_count != first_pass_line_count)
  {
    fprintf(
      stderr,
      "Source file changed between assembler passes: "
      "first pass read %zu line(s), "
      "second pass read %zu line(s).\n",
      first_pass_line_count,
      second_pass_line_count
    );

    return EXIT_FAILURE;
  }

  if (
    second_pass_result.program_size !=
    first_pass_result.program_size
  )
  {
    fprintf(
      stderr,
      "Assembler pass mismatch: "
      "first pass calculated %zu byte(s), "
      "second pass generated %zu byte(s).\n",
      first_pass_result.program_size,
      second_pass_result.program_size
    );

    return EXIT_FAILURE;
  }

  printf(
    "Second pass completed: %zu line(s), "
    "%zu instruction(s), %zu byte(s).\n",
    second_pass_line_count,
    second_pass_result.instruction_count,
    second_pass_result.program_size
  );

  const bool output_written =
    binary_writer_write(
        output_path,
        program,
        second_pass_result.program_size
    );

  if (!output_written)
  {
    return EXIT_FAILURE;
  }

  printf(
      "Wrote %zu byte(s) to %s. \n",
      second_pass_result.program_size,
      output_path
  );

  return EXIT_SUCCESS;
}
