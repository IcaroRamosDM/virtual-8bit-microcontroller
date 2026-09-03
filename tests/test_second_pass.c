#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "cpu.h"
#include "second_pass.h"
#include "symbol_table.h"

enum
{
  START_ADDRESS = 0x00,
  MEMORY_DEMO_ADDRESS = 0x09,
  COMPARISON_VALUE = 0x2A,
  FALLTHROUGH_VALUE = 0xFF,
  STORED_VALUE = 0x5A,
  CLEARED_VALUE = 0x00,
  DATA_ADDRESS = 0x80,
  EXPECTED_DEMO_INSTRUCTION_COUNT = 10,
  SINGLE_BYTE_PROGRAM_CAPACITY = 1,
  FIRST_PROGRAM_BYTE_INDEX = 0
};

typedef struct TestStatement
{
  size_t line_number;
  const char *text;
} TestStatement;

static const char TEST_INPUT_PATH[] =
  "programs/demo.asm";

static const TestStatement DEMO_STATEMENTS[] =
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

static const uint8_t EXPECTED_DEMO_PROGRAM[] =
{
  (uint8_t)OPCODE_LOAD_IMMEDIATE_A,
  COMPARISON_VALUE,

  (uint8_t)OPCODE_LOAD_IMMEDIATE_B,
  COMPARISON_VALUE,

  (uint8_t)OPCODE_SUB_A_B,

  (uint8_t)OPCODE_JUMP_IF_ZERO,
  MEMORY_DEMO_ADDRESS,

  (uint8_t)OPCODE_LOAD_IMMEDIATE_A,
  FALLTHROUGH_VALUE,

  (uint8_t)OPCODE_LOAD_IMMEDIATE_A,
  STORED_VALUE,

  (uint8_t)OPCODE_STORE_A_TO_MEMORY,
  DATA_ADDRESS,

  (uint8_t)OPCODE_LOAD_IMMEDIATE_A,
  CLEARED_VALUE,

  (uint8_t)OPCODE_LOAD_A_FROM_MEMORY,
  DATA_ADDRESS,

  (uint8_t)OPCODE_HALT
};

static void initialize_test_symbols(SymbolTable *symbols)
{
  symbol_table_initialize(symbols);

  SymbolTableAddResult add_result =
    symbol_table_add(
      symbols,
      "start",
      START_ADDRESS
    );

  assert(add_result == SYMBOL_TABLE_ADD_SUCCESS);

  add_result =
    symbol_table_add(
      symbols,
      "memory_demo",
      MEMORY_DEMO_ADDRESS
    );

  assert(add_result == SYMBOL_TABLE_ADD_SUCCESS);
}

static void process_demo_statements(
    SecondPassResult *result
)
{
  const size_t statement_count =
    sizeof DEMO_STATEMENTS /
    sizeof DEMO_STATEMENTS[0];

  for (
    size_t index = 0;
    index < statement_count;
    ++index
  )
  {
    const bool succeeded =
      second_pass_process_statement(
        TEST_INPUT_PATH,
        DEMO_STATEMENTS[index].line_number,
        DEMO_STATEMENTS[index].text,
        result
      );

    assert(succeeded);
  }
}

static void test_encodes_demo_program(void)
{
  SymbolTable symbols;

  initialize_test_symbols(&symbols);

  uint8_t program[CPU_MEMORY_SIZE] = {0};
  SecondPassResult result;

  second_pass_initialize(
    &result,
    &symbols,
    program,
    sizeof program
  );

  process_demo_statements(&result);

  const size_t expected_program_size =
    sizeof EXPECTED_DEMO_PROGRAM /
    sizeof EXPECTED_DEMO_PROGRAM[0];

  assert(result.symbols == &symbols);
  assert(result.program == program);
  assert(result.program_capacity == sizeof program);
  assert(result.program_size == expected_program_size);
  assert(
    result.instruction_count ==
    EXPECTED_DEMO_INSTRUCTION_COUNT
  );

  for (
    size_t index = 0;
    index < expected_program_size;
    ++index
  )
  {
    assert(
      program[index] ==
      EXPECTED_DEMO_PROGRAM[index]
    );
  }
}

static void test_accepts_exact_buffer_capacity(void)
{
  SymbolTable symbols;

  symbol_table_initialize(&symbols);

  uint8_t program[SINGLE_BYTE_PROGRAM_CAPACITY] =
  {
    CLEARED_VALUE
  };

  SecondPassResult result;

  second_pass_initialize(
    &result,
    &symbols,
    program,
    sizeof program
  );

  const bool succeeded =
    second_pass_process_statement(
      TEST_INPUT_PATH,
      1,
      "NOP",
      &result
    );

  assert(succeeded);
  assert(
    result.program_size ==
    SINGLE_BYTE_PROGRAM_CAPACITY
  );
  assert(result.instruction_count == 1);
  assert(
    program[FIRST_PROGRAM_BYTE_INDEX] ==
    (uint8_t)OPCODE_NOP
  );
}

int main(void)
{
  test_encodes_demo_program();
  test_accepts_exact_buffer_capacity();

  puts("All second-pass tests passed.");

  return EXIT_SUCCESS;
}
