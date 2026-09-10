#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "symbol_table.h"
#include "diagnostics.h"

typedef struct FirstPassResult
{
  AssemblerDiagnostics diagnostics;
  SymbolTable symbols;
  size_t statement_count;
  size_t program_size;
} FirstPassResult;

void first_pass_initialize(FirstPassResult *result);

bool first_pass_process_statement(
    const char *input_path,
    size_t line_number,
    const char *statement,
    void *context
);
