#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "cpu.h"
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
    {12, "CMP A, B"},
    {13, "JZ target"},
    {14, "JNZ target"},
    {15, "JC target"},
    {16, "JMP target"},
    {17, "LDA 0x80"},
    {18, "STA 0x80"},
    {19, "PUSH A"},
    {20, "POP A"},
    {21, "CALL target"},
    {22, "RET"}
  };

  enum
  {
    EXPECTED_PROGRAM_SIZE = 30
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
    {3, ".EQU COMPARISON_VALUE, 0x2A"},
    {4, ".EQU FALLTHROUGH_VALUE, 0xFF"},
    {5, ".EQU STORED_VALUE, 0x5A"},
    {6, ".EQU CLEARED_VALUE, 0x00"},
    {7, ".EQU DATA_ADDRESS, 0x80"},
    {9, "start:"},
    {10, "LDI A, COMPARISON_VALUE"},
    {11, "LDI B, COMPARISON_VALUE"},
    {12, "SUB A, B"},
    {13, "JZ memory_demo"},
    {14, "LDI A, FALLTHROUGH_VALUE"},
    {16, "memory_demo:"},
    {17, "LDA initial_data"},
    {18, "STA DATA_ADDRESS"},
    {19, "LDI A, CLEARED_VALUE"},
    {20, "LDA DATA_ADDRESS"},
    {21, "HALT"},
    {23, "initial_data:"},
    {24, ".BYTE STORED_VALUE"}
  };

  enum
  {
    EXPECTED_PROGRAM_SIZE = 19,
    EXPECTED_SYMBOL_COUNT = 8,
    START_ADDRESS = 0x00,
    MEMORY_DEMO_ADDRESS = 0x09,
    INITIAL_DATA_ADDRESS = 0x12
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

  const bool initial_data_was_found =
    symbol_table_find(
      &result.symbols,
      "initial_data",
      &address
    );

  assert(initial_data_was_found);
  assert(address == INITIAL_DATA_ADDRESS);
}

static void test_processes_equ_and_byte_directives(void)
{
  static const TestStatement statements[] =
  {
    {1, ".EQU DATA_ADDRESS, 0x80"},
    {2, ".EQU INITIAL_VALUE, 42"},
    {3, "start:"},
    {4, "LDI A, INITIAL_VALUE"},
    {5, ".BYTE DATA_ADDRESS"},
    {6, "HALT"}
  };

  enum
  {
    EXPECTED_PROGRAM_SIZE = 4,
    EXPECTED_SYMBOL_COUNT = 3,
    DATA_ADDRESS = 0x80,
    INITIAL_VALUE = 0x2A,
    START_ADDRESS = 0x00
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

  uint8_t value = 0;

  const bool data_address_was_found =
    symbol_table_find(
      &result.symbols,
      "DATA_ADDRESS",
      &value
    );

  assert(data_address_was_found);
  assert(value == DATA_ADDRESS);

  const bool initial_value_was_found =
    symbol_table_find(
      &result.symbols,
      "INITIAL_VALUE",
      &value
    );

  assert(initial_value_was_found);
  assert(value == INITIAL_VALUE);

  const bool start_was_found =
    symbol_table_find(
      &result.symbols,
      "start",
      &value
    );

  assert(start_was_found);
  assert(value == START_ADDRESS);
}

static void test_rejects_invalid_directives(void)
{
  static const TestStatement statements[] =
  {
    {1, ".EQU VALUE"},
    {2, ".EQU 1VALUE, 1"},
    {3, ".EQU VALUE, 256"},
    {4, ".BYTE"},
    {5, ".BYTE 1, 2"}
  };

  const size_t statement_count =
    sizeof statements / sizeof statements[0];

  for (
    size_t index = 0;
    index < statement_count;
    ++index
  )
  {
    FirstPassResult result;

    first_pass_initialize(&result);

    const bool succeeded =
      first_pass_process_statement(
        TEST_INPUT_PATH,
        statements[index].line_number,
        statements[index].text,
        &result
      );

    assert(!succeeded);
    assert(result.statement_count == 1);
    assert(result.program_size == 0);
    assert(result.symbols.count == 0);
  }
}

static void test_rejects_duplicate_label_and_constant(void)
{
  FirstPassResult result;

  first_pass_initialize(&result);

  const bool constant_succeeded =
    first_pass_process_statement(
      TEST_INPUT_PATH,
      1,
      ".EQU start, 0x10",
      &result
    );

  const bool label_succeeded =
    first_pass_process_statement(
      TEST_INPUT_PATH,
      2,
      "start:",
      &result
    );

  assert(constant_succeeded);
  assert(!label_succeeded);
  assert(result.symbols.count == 1);
}

static void test_rejects_program_that_reaches_stack_region(void)
{
  FirstPassResult result;

  first_pass_initialize(&result);

  for (
    size_t index = 0;
    index < CPU_PROGRAM_MEMORY_SIZE;
    ++index
  )
  {
    const bool succeeded =
      first_pass_process_statement(
        TEST_INPUT_PATH,
        index + 1,
        "NOP",
        &result
      );

    assert(succeeded);
  }

  const bool oversized_succeeded =
    first_pass_process_statement(
      TEST_INPUT_PATH,
      CPU_PROGRAM_MEMORY_SIZE + 1,
      "NOP",
      &result
    );

  assert(!oversized_succeeded);
  assert(result.program_size == CPU_PROGRAM_MEMORY_SIZE);
}

int main(void)
{
  test_recognizes_all_instruction_sizes();
  test_processes_demo_program();
  test_processes_equ_and_byte_directives();
  test_rejects_invalid_directives();
  test_rejects_duplicate_label_and_constant();
  test_rejects_program_that_reaches_stack_region();

  puts("All first-pass tests passed.");

  return EXIT_SUCCESS;
}
