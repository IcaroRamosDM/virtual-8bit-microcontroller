#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "instruction_parser.h"

typedef struct InvalidInstructionTestCase
{
  const char *statement;
  InstructionParseResult expected_result;
} InvalidInstructionTestCase;

static void assert_successful_parse(
    const char *statement,
    const char *expected_mnemonic,
    size_t expected_operand_count,
    const char *expected_first_operand,
    const char *expected_second_operand
)
{
  ParsedInstruction instruction;

  const InstructionParseResult result =
    instruction_parse(statement, &instruction);

  assert(result == INSTRUCTION_PARSE_SUCCESS);
  assert(
    strcmp(
      instruction.mnemonic,
      expected_mnemonic
    ) == 0
  );
  assert(
    instruction.operand_count ==
    expected_operand_count
  );

  if (expected_operand_count > 0)
  {
    assert(expected_first_operand != NULL);
    assert(
      strcmp(
        instruction.operands[0],
        expected_first_operand
      ) == 0
    );
  }

  if (expected_operand_count > 1)
  {
    assert(expected_second_operand != NULL);
    assert(
      strcmp(
        instruction.operands[1],
        expected_second_operand
      ) == 0
    );
  }
}

static void test_parses_instruction_without_operands(void)
{
  assert_successful_parse(
    "NOP",
    "NOP",
    0,
    NULL,
    NULL
  );
}

static void test_parses_instruction_with_one_operand(void)
{
  assert_successful_parse(
    "JZ memory_demo",
    "JZ",
    1,
    "memory_demo",
    NULL
  );
}

static void test_parses_instruction_with_two_operands(void)
{
  assert_successful_parse(
    "LDI A, 0x2A",
    "LDI",
    2,
    "A",
    "0x2A"
  );
}

static void test_accepts_whitespace_around_comma(void)
{
  assert_successful_parse(
    "  ADD   A ,  B  ",
    "ADD",
    2,
    "A",
    "B"
  );
}

static void test_accepts_comma_without_following_space(void)
{
  assert_successful_parse(
    "LDI B,42",
    "LDI",
    2,
    "B",
    "42"
  );
}

static void test_reports_invalid_syntax(void)
{
  static const InvalidInstructionTestCase test_cases[] =
  {
    {"", INSTRUCTION_PARSE_EMPTY},
    {"   ", INSTRUCTION_PARSE_EMPTY},
    {", A", INSTRUCTION_PARSE_EXPECTED_MNEMONIC},
    {"LDI,A", INSTRUCTION_PARSE_EXPECTED_OPERAND},
    {"LDI , 0x2A", INSTRUCTION_PARSE_EXPECTED_OPERAND},
    {"LDI A,", INSTRUCTION_PARSE_EXPECTED_OPERAND},
    {"ADD A B", INSTRUCTION_PARSE_EXPECTED_COMMA},
    {"ADD A, B, C", INSTRUCTION_PARSE_TOO_MANY_OPERANDS},
    {"ADD A, B C", INSTRUCTION_PARSE_TOO_MANY_OPERANDS}
  };

  const size_t test_case_count =
    sizeof test_cases / sizeof test_cases[0];

  for (
    size_t index = 0;
    index < test_case_count;
    ++index
  )
  {
    ParsedInstruction instruction;

    const InstructionParseResult result =
      instruction_parse(
        test_cases[index].statement,
        &instruction
      );

    assert(result == test_cases[index].expected_result);
  }
}

int main(void)
{
  test_parses_instruction_without_operands();
  test_parses_instruction_with_one_operand();
  test_parses_instruction_with_two_operands();
  test_accepts_whitespace_around_comma();
  test_accepts_comma_without_following_space();
  test_reports_invalid_syntax();

  puts("All instruction-parser tests passed.");

  return EXIT_SUCCESS;
}
