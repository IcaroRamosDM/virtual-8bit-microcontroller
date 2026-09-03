#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "first_pass.h"

typedef struct TestStatement
{
  size_t line_number;
  const char *text;
} TestStatement;

static const char TEST_INPUT_PATH[] =
  "programs/demo.asm";

static void process_statements(
    FirstPassResult *result,
    const TestStatement *statements,
    size_t statement_count
)
{
  for (
    size_t index = 0;
    index < statement_count;
    ++index
  )
  {
    const bool succeeded =
      first_pass_process_statement(
        TEST_INPUT_PATH,
        statements[index].line_number,
        statements[index].text,
        result
      );

    assert(succeeded);
  }
}

static void test_recognizes_all_instruction_sizes(void)
{
  static const TestStatement statements[] =
  {
    {1, "NOP"},
    {2, "HALT"},
    {3, "LDI A, 0x00"},
    {4, "ADD A, B"},
    {5, "SUB A, B"},
    {6, "AND A, B"},
    {7, "OR A, B"},
    {8, "XOR A, B"},
    {9, "NOT A"},
    {10, "SHL A"},
    {11, "SHR A"},
    {12, "JZ target"},
    {13, "JMP target"},
    {14, "LDA 0x80"},
    {15, "STA 0x80"}
  };

  enum
  {
    EXPECTED_PROGRAM_SIZE = 20
  };

  const size_t statement_count =
    sizeof statements / sizeof statements[0];

  FirstPassResult result;

  first_pass_initialize(&result);

  process_statements(
    &result,
    statements,
    statement_count
  );

  assert(result.statement_count == statement_count);
  assert(result.program_size == EXPECTED_PROGRAM_SIZE);
  assert(result.symbols.count == 0);
}

static void test_processes_demo_program(void)
{
  static const TestStatement statements[] =
  {
    {3, "start:"},
    {4, "LDI A, 0x2A"},
    {5, "LDI B, 0x2A"},
    {6, "SUB A, B"},
    {7, "JZ memory_demo"},
    {8, "LDI A, 0xFF"},
    {10, "memory_demo:"},
    {11, "LDI A, 0x5A"},
    {12, "STA 0x80"},
    {13, "LDI A, 0x00"},
    {14, "LDA 0x80"},
    {15, "HALT"}
  };

  enum
  {
    EXPECTED_PROGRAM_SIZE = 18,
    EXPECTED_SYMBOL_COUNT = 2,
    START_ADDRESS = 0x00,
    MEMORY_DEMO_ADDRESS = 0x09
  };

  const size_t statement_count =
    sizeof statements / sizeof statements[0];

  FirstPassResult result;

  first_pass_initialize(&result);

  process_statements(
    &result,
    statements,
    statement_count
  );

  assert(result.statement_count == statement_count);
  assert(result.program_size == EXPECTED_PROGRAM_SIZE);
  assert(result.symbols.count == EXPECTED_SYMBOL_COUNT);

  uint8_t address = 0;

  const bool start_was_found =
    symbol_table_find(
      &result.symbols,
      "start",
      &address
    );

  assert(start_was_found);
  assert(address == START_ADDRESS);

  const bool memory_demo_was_found =
    symbol_table_find(
      &result.symbols,
      "memory_demo",
      &address
    );

  assert(memory_demo_was_found);
  assert(address == MEMORY_DEMO_ADDRESS);
}

int main(void)
{
  test_recognizes_all_instruction_sizes();
  test_processes_demo_program();

  puts("All first-pass tests passed.");

  return EXIT_SUCCESS;
}
