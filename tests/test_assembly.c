#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "assembly.h"
#include "source_reader.h"

enum { DIAGNOSTIC_CAPACITY = 4096, LINE_SIZE = 4 };

static void collect_message(const char *message, void *context)
{
  char *text = context;
  const size_t used = strlen(text);
  const size_t remaining = DIAGNOSTIC_CAPACITY - used - 1;
  strncat(text, message, remaining);
}

static void test_valid_image_and_source_map(void)
{
  const char source[] = "; example\r\n.EQU VALUE, 0x2A\r\nstart:\r\nLDI A, VALUE\r\nHALT\r\n.BYTE start";
  AssemblyImage image;
  char messages[DIAGNOSTIC_CAPACITY] = {0};
  const AssemblerDiagnostics diagnostics = {collect_message, messages};
  const bool success = assembly_compile(source, &image, diagnostics);
  const uint8_t expected[] = {OPCODE_LOAD_IMMEDIATE_A, 0x2A, OPCODE_HALT, 0};
  assert(success);
  assert(messages[0] == '\0');
  assert(image.byte_count == sizeof expected);
  assert(memcmp(image.bytes, expected, sizeof expected) == 0);
  assert(image.instruction_count == 2);
  assert(image.symbol_count == 2);
  assert(image.line_count == 6);
  assert(image.line_for_address[0] == 4);
  assert(image.line_for_address[1] == 4);
  assert(image.line_for_address[2] == 5);
  assert(image.line_for_address[3] == 6);
  assert(image.instruction_start[0]);
  assert(!image.instruction_start[1]);
  assert(image.instruction_start[2]);
  assert(!image.instruction_start[3]);
}

static void test_errors_are_captured_and_output_is_cleared(void)
{
  const char *sources[] = {"HALT\nBAD\n", "HALT\nLDI A 1\n", "HALT\nJMP missing\n", "HALT\nLDI A, 256\n"};
  for (size_t i = 0; i < sizeof sources / sizeof sources[0]; ++i)
  {
    AssemblyImage image = {0};
    image.byte_count = 1;
    char messages[DIAGNOSTIC_CAPACITY] = {0};
    const bool success = assembly_compile(sources[i], &image,
        (AssemblerDiagnostics){collect_message, messages});
    assert(!success);
    assert(image.byte_count == 0);
    assert(image.error_line == 2);
    assert(strstr(messages, "Editor:2:") != NULL);
  }
}

static void test_capacity_and_empty_input(void)
{
  char source[(CPU_PROGRAM_MEMORY_SIZE + 1) * LINE_SIZE + 1] = {0};
  for (size_t i = 0; i < CPU_PROGRAM_MEMORY_SIZE; ++i)
  {
    memcpy(source + i * LINE_SIZE, "NOP\n", LINE_SIZE);
  }
  AssemblyImage image;
  char messages[DIAGNOSTIC_CAPACITY] = {0};
  const AssemblerDiagnostics diagnostics = {collect_message, messages};
  bool success = assembly_compile(source, &image, diagnostics);
  assert(success);
  assert(image.byte_count == CPU_PROGRAM_MEMORY_SIZE);
  memcpy(source + CPU_PROGRAM_MEMORY_SIZE * LINE_SIZE, "NOP\n", LINE_SIZE);
  success = assembly_compile(source, &image, diagnostics);
  assert(!success);
  assert(image.error_line == CPU_PROGRAM_MEMORY_SIZE + 1);
  success = assembly_compile("; nothing\n", &image, diagnostics);
  assert(!success);
  assert(strstr(messages, "program is empty") != NULL);
  success = assembly_compile(NULL, &image, diagnostics);
  assert(!success);
  success = assembly_compile("HALT", NULL, diagnostics);
  assert(!success);
}

static void test_source_and_line_limits(void)
{
  char messages[DIAGNOSTIC_CAPACITY] = {0};
  const AssemblerDiagnostics diagnostics = {collect_message, messages};
  AssemblyImage image;
  char line[SOURCE_READER_MAX_LINE_LENGTH + 8];
  memset(line, ' ', sizeof line);
  memcpy(line, "HALT ;", strlen("HALT ;"));
  line[SOURCE_READER_MAX_LINE_LENGTH] = '\0';
  bool success = assembly_compile(line, &image, diagnostics);
  assert(success);
  line[SOURCE_READER_MAX_LINE_LENGTH] = ' ';
  line[SOURCE_READER_MAX_LINE_LENGTH + 1] = '\0';
  success = assembly_compile(line, &image, diagnostics);
  assert(!success);
  assert(image.error_line == 1);

  static char source[ASSEMBLY_SOURCE_CAPACITY + 2];
  memset(source, '\n', sizeof source);
  memcpy(source, "HALT", strlen("HALT"));
  source[ASSEMBLY_SOURCE_CAPACITY] = '\0';
  success = assembly_compile(source, &image, diagnostics);
  assert(success);
  source[ASSEMBLY_SOURCE_CAPACITY] = '\n';
  source[ASSEMBLY_SOURCE_CAPACITY + 1] = '\0';
  success = assembly_compile(source, &image, diagnostics);
  assert(!success);
  assert(image.byte_count == 0);
}

int main(void)
{
  test_valid_image_and_source_map();
  test_errors_are_captured_and_output_is_cleared();
  test_capacity_and_empty_input();
  test_source_and_line_limits();
  puts("All in-memory assembly tests passed.");
  return 0;
}
