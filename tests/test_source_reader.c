#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "source_reader.h"

enum
{
  TEST_CAPTURE_CAPACITY = 4,
  TEST_STATEMENT_BUFFER_SIZE =
    SOURCE_READER_MAX_LINE_LENGTH + 1,
  TEST_OVERSIZED_LINE_LENGTH =
    SOURCE_READER_MAX_LINE_LENGTH + 1
};

static const char TEST_SOURCE_PATH[] =
  "build/test_source_reader_input.asm";

typedef struct CapturedStatement
{
  size_t line_number;
  char text[TEST_STATEMENT_BUFFER_SIZE];
} CapturedStatement;

typedef struct StatementCapture
{
  CapturedStatement statements[TEST_CAPTURE_CAPACITY];
  size_t count;
} StatementCapture;

static bool capture_statement(
    const char *input_path,
    size_t line_number,
    const char *statement,
    void *context
)
{
  StatementCapture *capture = context;

  assert(strcmp(input_path, TEST_SOURCE_PATH) == 0);
  assert(capture->count < TEST_CAPTURE_CAPACITY);

  CapturedStatement *captured_statement =
    &capture->statements[capture->count];

  captured_statement->line_number = line_number;

  const int written_character_count = snprintf(
    captured_statement->text,
    sizeof captured_statement->text,
    "%s",
    statement
  );

  assert(written_character_count >= 0);
  assert(
    (size_t)written_character_count <
    sizeof captured_statement->text
  );

  ++capture->count;

  return true;
}

static void write_test_source(const char *contents)
{
  FILE *source = fopen(TEST_SOURCE_PATH, "w");

  assert(source != NULL);

  const int write_result = fputs(contents, source);

  assert(write_result != EOF);

  const int close_result = fclose(source);

  assert(close_result == 0);
}

static void remove_test_source(void)
{
  const int remove_result = remove(TEST_SOURCE_PATH);

  assert(remove_result == 0);
}

static void test_reads_and_normalizes_statements(void)
{
  static const char source_text[] =
    "; Header comment.\n"
    "\n"
    "  LDI A, 0x2A ; Load value.\n"
    "HALT\n";

  StatementCapture capture = {0};
  size_t line_count = 0;

  write_test_source(source_text);

  const bool read_succeeded = source_reader_read(
    TEST_SOURCE_PATH,
    capture_statement,
    &capture,
    &line_count
  );

  assert(read_succeeded);
  assert(line_count == 4);
  assert(capture.count == 2);

  assert(capture.statements[0].line_number == 3);
  assert(
    strcmp(
      capture.statements[0].text,
      "LDI A, 0x2A"
    ) == 0
  );

  assert(capture.statements[1].line_number == 4);
  assert(
    strcmp(
      capture.statements[1].text,
      "HALT"
    ) == 0
  );

  remove_test_source();
}

static void test_accepts_final_line_without_newline(void)
{
  static const char source_text[] =
    "  JMP start ; Final statement.";

  StatementCapture capture = {0};
  size_t line_count = 0;

  write_test_source(source_text);

  const bool read_succeeded = source_reader_read(
    TEST_SOURCE_PATH,
    capture_statement,
    &capture,
    &line_count
  );

  assert(read_succeeded);
  assert(line_count == 1);
  assert(capture.count == 1);
  assert(capture.statements[0].line_number == 1);
  assert(
    strcmp(
      capture.statements[0].text,
      "JMP start"
    ) == 0
  );

  remove_test_source();
}

static void test_rejects_oversized_line(void)
{
  char source_text[TEST_OVERSIZED_LINE_LENGTH + 2];

  memset(
    source_text,
    'A',
    TEST_OVERSIZED_LINE_LENGTH
  );

  source_text[TEST_OVERSIZED_LINE_LENGTH] = '\n';
  source_text[TEST_OVERSIZED_LINE_LENGTH + 1] = '\0';

  StatementCapture capture = {0};
  size_t line_count = 0;

  write_test_source(source_text);

  const bool read_succeeded = source_reader_read(
    TEST_SOURCE_PATH,
    capture_statement,
    &capture,
    &line_count
  );

  assert(!read_succeeded);
  assert(line_count == 1);
  assert(capture.count == 0);

  remove_test_source();
}

int main(void)
{
  test_reads_and_normalizes_statements();
  test_accepts_final_line_without_newline();
  test_rejects_oversized_line();

  puts("All source-reader tests passed.");

  return EXIT_SUCCESS;
}
