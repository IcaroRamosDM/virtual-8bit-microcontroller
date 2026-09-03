#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "cpu.h"
#include "instruction_encoder.h"
#include "instruction_parser.h"
#include "symbol_table.h"

enum
{
  START_ADDRESS = 0x00,
  MEMORY_DEMO_ADDRESS = 0x09,
  IMMEDIATE_VALUE = 0x2A,
  DATA_ADDRESS = 0x80,
  UNUSED_BYTE = 0x00,
  SENTINEL_BYTE = 0xA5,
  NO_OPERAND_COUNT = 0,
  ONE_OPERAND_COUNT = 1,
  ONE_BYTE_INSTRUCTION_SIZE = 1,
  TWO_BYTE_INSTRUCTION_SIZE = 2,
  SENTINEL_SIZE = INSTRUCTION_MAX_ENCODED_SIZE
};

typedef struct SuccessfulEncodingTestCase
{
  const char *statement;
  uint8_t expected_bytes[INSTRUCTION_MAX_ENCODED_SIZE];
  size_t expected_size;
} SuccessfulEncodingTestCase;

typedef struct EncodingErrorTestCase
{
  const char *statement;
  InstructionEncodeResult expected_result;
} EncodingErrorTestCase;

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

static void test_encodes_supported_instructions(
    const SymbolTable *symbols
)
{
  static const SuccessfulEncodingTestCase test_cases[] =
  {
    {
      "NOP",
      {(uint8_t)OPCODE_NOP, UNUSED_BYTE},
      ONE_BYTE_INSTRUCTION_SIZE
    },
    {
      "HALT",
      {(uint8_t)OPCODE_HALT, UNUSED_BYTE},
      ONE_BYTE_INSTRUCTION_SIZE
    },
    {
      "LDI A, 0x2A",
      {(uint8_t)OPCODE_LOAD_IMMEDIATE_A, IMMEDIATE_VALUE},
      TWO_BYTE_INSTRUCTION_SIZE
    },
    {
      "LDI B, 42",
      {(uint8_t)OPCODE_LOAD_IMMEDIATE_B, IMMEDIATE_VALUE},
      TWO_BYTE_INSTRUCTION_SIZE
    },
    {
      "ADD A, B",
      {(uint8_t)OPCODE_ADD_A_B, UNUSED_BYTE},
      ONE_BYTE_INSTRUCTION_SIZE
    },
    {
      "SUB A, B",
      {(uint8_t)OPCODE_SUB_A_B, UNUSED_BYTE},
      ONE_BYTE_INSTRUCTION_SIZE
    },
    {
      "JZ memory_demo",
      {(uint8_t)OPCODE_JUMP_IF_ZERO, MEMORY_DEMO_ADDRESS},
      TWO_BYTE_INSTRUCTION_SIZE
    },
    {
      "JMP start",
      {(uint8_t)OPCODE_JUMP, START_ADDRESS},
      TWO_BYTE_INSTRUCTION_SIZE
    },
    {
      "LDA 0x80",
      {(uint8_t)OPCODE_LOAD_A_FROM_MEMORY, DATA_ADDRESS},
      TWO_BYTE_INSTRUCTION_SIZE
    },
    {
      "STA 128",
      {(uint8_t)OPCODE_STORE_A_TO_MEMORY, DATA_ADDRESS},
      TWO_BYTE_INSTRUCTION_SIZE
    }
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

    const InstructionParseResult parse_result =
      instruction_parse(
        test_cases[index].statement,
        &instruction
      );

    assert(parse_result == INSTRUCTION_PARSE_SUCCESS);

    EncodedInstruction encoded_instruction = {0};

    const InstructionEncodeResult encode_result =
      instruction_encode(
        &instruction,
        symbols,
        &encoded_instruction
      );

    assert(encode_result == INSTRUCTION_ENCODE_SUCCESS);
    assert(
      encoded_instruction.size ==
      test_cases[index].expected_size
    );

    for (
      size_t byte_index = 0;
      byte_index < INSTRUCTION_MAX_ENCODED_SIZE;
      ++byte_index
    )
    {
      assert(
        encoded_instruction.bytes[byte_index] ==
        test_cases[index].expected_bytes[byte_index]
      );
    }
  }
}

static void test_reports_semantic_errors(
    const SymbolTable *symbols
)
{
  static const EncodingErrorTestCase test_cases[] =
  {
    {
      "MUL A, B",
      INSTRUCTION_ENCODE_UNKNOWN_MNEMONIC
    },
    {
      "NOP A",
      INSTRUCTION_ENCODE_WRONG_OPERAND_COUNT
    },
    {
      "JMP",
      INSTRUCTION_ENCODE_WRONG_OPERAND_COUNT
    },
    {
      "LDI C, 1",
      INSTRUCTION_ENCODE_INVALID_REGISTER
    },
    {
      "ADD B, A",
      INSTRUCTION_ENCODE_INVALID_REGISTER
    },
    {
      "JMP 256",
      INSTRUCTION_ENCODE_OPERAND_OUT_OF_RANGE
    },
    {
      "JZ missing",
      INSTRUCTION_ENCODE_UNDEFINED_SYMBOL
    },
    {
      "STA bad-name",
      INSTRUCTION_ENCODE_INVALID_OPERAND
    }
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

    const InstructionParseResult parse_result =
      instruction_parse(
        test_cases[index].statement,
        &instruction
      );

    assert(parse_result == INSTRUCTION_PARSE_SUCCESS);

    EncodedInstruction encoded_instruction =
    {
      {SENTINEL_BYTE, SENTINEL_BYTE},
      SENTINEL_SIZE
    };

    const InstructionEncodeResult encode_result =
      instruction_encode(
        &instruction,
        symbols,
        &encoded_instruction
      );

    assert(
      encode_result ==
      test_cases[index].expected_result
    );

    assert(
      encoded_instruction.size ==
      SENTINEL_SIZE
    );

    for (
      size_t byte_index = 0;
      byte_index < INSTRUCTION_MAX_ENCODED_SIZE;
      ++byte_index
    )
    {
      assert(
        encoded_instruction.bytes[byte_index] ==
        SENTINEL_BYTE
      );
    }
  }
}

static void test_reports_empty_operand(
    const SymbolTable *symbols
)
{
  ParsedInstruction instruction =
  {
    .mnemonic = "JMP",
    .operand_count = ONE_OPERAND_COUNT
  };

  EncodedInstruction encoded_instruction =
  {
    {SENTINEL_BYTE, SENTINEL_BYTE},
    SENTINEL_SIZE
  };

  const InstructionEncodeResult encode_result =
    instruction_encode(
      &instruction,
      symbols,
      &encoded_instruction
    );

  assert(
    encode_result ==
    INSTRUCTION_ENCODE_EMPTY_OPERAND
  );

  assert(encoded_instruction.size == SENTINEL_SIZE);
  assert(
    encoded_instruction.bytes[0] ==
    SENTINEL_BYTE
  );
  assert(
    encoded_instruction.bytes[1] ==
    SENTINEL_BYTE
  );
}

int main(void)
{
  SymbolTable symbols;

  initialize_test_symbols(&symbols);
  test_encodes_supported_instructions(&symbols);
  test_reports_semantic_errors(&symbols);
  test_reports_empty_operand(&symbols);

  puts("All instruction-encoder tests passed.");

  return EXIT_SUCCESS;
}
