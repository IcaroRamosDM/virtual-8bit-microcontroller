#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "symbol_table.h"
#include "diagnostics.h"

typedef struct SecondPassResult
{
  AssemblerDiagnostics diagnostics;
  const SymbolTable *symbols;
  uint8_t *program;
  size_t program_capacity;
  size_t program_size;
  size_t instruction_count;
} SecondPassResult;

void second_pass_initialize(
    SecondPassResult *result,
    const SymbolTable *symbols,
    uint8_t *program,
    size_t program_capacity
);

bool second_pass_process_statement(
    const char *input_path,
    size_t line_number,
    const char *statement,
    void *context
);
