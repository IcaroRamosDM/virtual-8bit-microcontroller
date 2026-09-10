#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "cpu.h"
#include "diagnostics.h"

enum { ASSEMBLY_SOURCE_CAPACITY = 65536 };

typedef struct AssemblyImage
{
  uint8_t bytes[CPU_PROGRAM_MEMORY_SIZE];
  size_t byte_count;
  size_t instruction_count;
  size_t symbol_count;
  size_t line_count;
  size_t line_for_address[CPU_PROGRAM_MEMORY_SIZE];
  bool instruction_start[CPU_PROGRAM_MEMORY_SIZE];
  size_t error_line;
} AssemblyImage;

/* No filesystem or process launch is needed. Output is cleared on failure. */
bool assembly_compile(
    const char *source,
    AssemblyImage *image,
    AssemblerDiagnostics diagnostics);
