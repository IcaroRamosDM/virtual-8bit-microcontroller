#include "assembly.h"

#include <string.h>

#include "first_pass.h"
#include "second_pass.h"
#include "source_line.h"
#include "source_reader.h"

static bool process_text(
    const char *source,
    FirstPassResult *first,
    SecondPassResult *second,
    AssemblyImage *image,
    AssemblerDiagnostics diagnostics)
{
  const char *cursor = source;
  size_t line_number = 0;

  while (*cursor != '\0')
  {
    ++line_number;
    const char *end = strchr(cursor, '\n');
    size_t length = end == NULL ? strlen(cursor) : (size_t)(end - cursor);
    if (length > 0 && cursor[length - 1] == '\r')
    {
      --length;
    }
    if (length > SOURCE_READER_MAX_LINE_LENGTH)
    {
      image->error_line = line_number;
      assembler_report(&diagnostics, "Editor:%zu: source line exceeds %d characters\n",
          line_number, SOURCE_READER_MAX_LINE_LENGTH);
      return false;
    }

    char line[SOURCE_READER_MAX_LINE_LENGTH + 1];
    memcpy(line, cursor, length);
    line[length] = '\0';
    source_line_normalize(line);
    if (*line != '\0')
    {
      bool succeeded;
      if (first != NULL)
      {
        succeeded = first_pass_process_statement("Editor", line_number, line, first);
      }
      else
      {
        const size_t start = second->program_size;
        const size_t instruction_count = second->instruction_count;
        succeeded = second_pass_process_statement("Editor", line_number, line, second);
        if (succeeded)
        {
          for (size_t address = start; address < second->program_size; ++address)
          {
            image->line_for_address[address] = line_number;
          }
          if (second->instruction_count > instruction_count)
          {
            image->instruction_start[start] = true;
          }
        }
      }
      if (!succeeded)
      {
        image->error_line = line_number;
        return false;
      }
    }
    cursor = end == NULL ? cursor + strlen(cursor) : end + 1;
  }
  image->line_count = line_number;
  return true;
}

bool assembly_compile(
    const char *source,
    AssemblyImage *image,
    AssemblerDiagnostics diagnostics)
{
  if (image == NULL)
  {
    return false;
  }
  *image = (AssemblyImage){0};
  if (source == NULL || strlen(source) > ASSEMBLY_SOURCE_CAPACITY)
  {
    assembler_report(&diagnostics, "Editor: source is missing or exceeds %d bytes\n",
        ASSEMBLY_SOURCE_CAPACITY);
    return false;
  }

  AssemblyImage candidate = {0};
  FirstPassResult first;
  first_pass_initialize(&first);
  first.diagnostics = diagnostics;
  if (!process_text(source, &first, NULL, &candidate, diagnostics))
  {
    image->error_line = candidate.error_line;
    return false;
  }
  if (first.program_size == 0)
  {
    assembler_report(&diagnostics, "Editor: program is empty; add an instruction such as HALT\n");
    return false;
  }

  SecondPassResult second;
  second_pass_initialize(&second, &first.symbols, candidate.bytes, sizeof candidate.bytes);
  second.diagnostics = diagnostics;
  if (!process_text(source, NULL, &second, &candidate, diagnostics))
  {
    image->error_line = candidate.error_line;
    return false;
  }
  if (first.program_size != second.program_size)
  {
    assembler_report(&diagnostics, "Editor: assembler pass sizes do not match\n");
    return false;
  }
  candidate.byte_count = second.program_size;
  candidate.instruction_count = second.instruction_count;
  candidate.symbol_count = first.symbols.count;
  *image = candidate;
  return true;
}
