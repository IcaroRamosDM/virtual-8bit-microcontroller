#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "source_line.h"

static void test_preserves_normalized_instruction(void)
{
  char line[] = "LDI A, 0x2A";

  source_line_normalize(line);

  assert(strcmp(line, "LDI A, 0x2A") == 0);
}

static void test_removes_surrounding_whitespace(void)
{
  char line[] = " \tLDI B, 42 \r\n";

  source_line_normalize(line);

  assert(strcmp(line, "LDI B, 42") == 0);
}

static void test_removes_trailing_comment(void)
{
  char line[] = "ADD A, B ; Update accumulator.\n";

  source_line_normalize(line);

  assert(strcmp(line, "ADD A, B") == 0);
}

static void test_removes_comment_without_preceding_space(void)
{
  char line[] = "HALT; End execution.\n";

  source_line_normalize(line);

  assert(strcmp(line, "HALT") == 0);
}

static void test_clears_comment_only_line(void)
{
  char line[] = "  ; Comment only.\n";

  source_line_normalize(line);

  assert(strcmp(line, "") == 0);
}

static void test_clears_blank_line(void)
{
  char line[] = "\t \r\n";

  source_line_normalize(line);

  assert(strcmp(line, "") == 0);
}

int main(void)
{
  test_preserves_normalized_instruction();
  test_removes_surrounding_whitespace();
  test_removes_trailing_comment();
  test_removes_comment_without_preceding_space();
  test_clears_comment_only_line();
  test_clears_blank_line();

  puts("All source-line tests passed.");

  return 0;
}
