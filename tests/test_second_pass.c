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
  INITIAL_DATA_ADDRESS = 0x12,
  COMPARISON_VALUE = 0x2A,
  FALLTHROUGH_VALUE = 0xFF,
  STORED_VALUE = 0x5A,
  CLEARED_VALUE = 0x00,
  DATA_ADDRESS = 0x80,
  RAW_DATA_VALUE = 0xA5,
  EXPECTED_DEMO_INSTRUCTION_COUNT = 10,
  DIRECTIVE_PROGRAM_SIZE = 3,
  DIRECTIVE_INSTRUCTION_COUNT = 1,
  SINGLE_BYTE_PROGRAM_CAPACITY = 1,
  FIRST_PROGRAM_BYTE_INDEX = 0,
  SECOND_PROGRAM_BYTE_INDEX = 1,
  THIRD_PROGRAM_BYTE_INDEX = 2
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

  (uint8_t)OPCODE_LOAD_A_FROM_MEMORY,
  INITIAL_DATA_ADDRESS,

  (uint8_t)OPCODE_STORE_A_TO_MEMORY,
  DATA_ADDRESS,

  (uint8_t)OPCODE_LOAD_IMMEDIATE_A,
  CLEARED_VALUE,

  (uint8_t)OPCODE_LOAD_A_FROM_MEMORY,
  DATA_ADDRESS,

  (uint8_t)OPCODE_HALT,

  STORED_VALUE
};

static void add_test_symbol(
    SymbolTable *symbols,
    const char *name,
    uint8_t value
)
{
  const SymbolTableAddResult add_result =
    symbol_table_add(symbols, name, value);

  assert(add_result == SYMBOL_TABLE_ADD_SUCCESS);
}

static void initialize_test_symbols(SymbolTable *symbols)
{
  symbol_table_initialize(symbols);

  add_test_symbol(symbols, "COMPARISON_VALUE", COMPARISON_VALUE);
  add_test_symbol(symbols, "FALLTHROUGH_VALUE", FALLTHROUGH_VALUE);
  add_test_symbol(symbols, "STORED_VALUE", STORED_VALUE);
  add_test_symbol(symbols, "CLEARED_VALUE", CLEARED_VALUE);
  add_test_symbol(symbols, "DATA_ADDRESS", DATA_ADDRESS);
  add_test_symbol(symbols, "start", START_ADDRESS);
  add_test_symbol(symbols, "memory_demo", MEMORY_DEMO_ADDRESS);
  add_test_symbol(symbols, "initial_data", INITIAL_DATA_ADDRESS);
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

  uint8_t program[CPU_PROGRAM_MEMORY_SIZE] = {0};
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

static void test_emits_byte_directives_and_ignores_equ(void)
{
  SymbolTable symbols;

  symbol_table_initialize(&symbols);

  const SymbolTableAddResult add_result =
    symbol_table_add(
      &symbols,
      "RAW_VALUE",
      RAW_DATA_VALUE
    );

  assert(add_result == SYMBOL_TABLE_ADD_SUCCESS);

  uint8_t program[DIRECTIVE_PROGRAM_SIZE] = {0};
  SecondPassResult result;

  second_pass_initialize(
    &result,
    &symbols,
    program,
    sizeof program
  );

  static const TestStatement statements[] =
  {
    {1, ".EQU RAW_VALUE, 0xA5"},
    {2, ".BYTE RAW_VALUE"},
    {3, ".BYTE 0x5A"},
    {4, "HALT"}
  };

  const size_t statement_count =
    sizeof statements / sizeof statements[0];

  for (
    size_t index = 0;
    index < statement_count;
    ++index
  )
  {
    const bool succeeded =
      second_pass_process_statement(
        TEST_INPUT_PATH,
        statements[index].line_number,
        statements[index].text,
        &result
      );

    assert(succeeded);
  }

  assert(result.program_size == DIRECTIVE_PROGRAM_SIZE);
  assert(
    result.instruction_count ==
    DIRECTIVE_INSTRUCTION_COUNT
  );
  assert(
    program[FIRST_PROGRAM_BYTE_INDEX] ==
    RAW_DATA_VALUE
  );
  assert(
    program[SECOND_PROGRAM_BYTE_INDEX] ==
    STORED_VALUE
  );
  assert(
    program[THIRD_PROGRAM_BYTE_INDEX] ==
    (uint8_t)OPCODE_HALT
  );
}

int main(void)
{
  test_encodes_demo_program();
  test_accepts_exact_buffer_capacity();
  test_emits_byte_directives_and_ignores_equ();

  puts("All second-pass tests passed.");

  return EXIT_SUCCESS;
}
