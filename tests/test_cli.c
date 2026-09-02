#include <assert.h>
#include <stdio.h>

#include "cli.h"

static void test_cli_parse_command_runs_without_command(void)
{
  char program_name[] = "vm8";
  char *arguments[] = {
    program_name
  };
  const int argument_count =
    (int)(sizeof arguments / sizeof arguments[0]);

  const CliCommand command = cli_parse_command(
    argument_count,
    arguments
  );

  assert(command == CLI_COMMAND_RUN);
}

static void test_cli_parse_command_accepts_help_commands(void)
{
  char program_name[] = "vm8";
  char help_command[] = "help";
  char long_help_command[] = "--help";

  char *help_arguments[] = {
    program_name,
    help_command
  };
  char *long_help_arguments[] = {
    program_name,
    long_help_command
  };

  const int help_argument_count =
    (int)(sizeof help_arguments / sizeof help_arguments[0]);
  const int long_help_argument_count =
    (int)(sizeof long_help_arguments /
          sizeof long_help_arguments[0]);

  const CliCommand help_result = cli_parse_command(
    help_argument_count,
    help_arguments
  );
  const CliCommand long_help_result = cli_parse_command(
    long_help_argument_count,
    long_help_arguments
  );

  assert(help_result == CLI_COMMAND_HELP);
  assert(long_help_result == CLI_COMMAND_HELP);
}

static void test_cli_parse_command_rejects_invalid_commands(void)
{
  char program_name[] = "vm8";
  char invalid_command[] = "unknown";
  char help_command[] = "help";
  char extra_argument[] = "extra";

  char *invalid_arguments[] = {
    program_name,
    invalid_command
  };
  char *too_many_arguments[] = {
    program_name,
    help_command,
    extra_argument
  };

  const int invalid_argument_count =
    (int)(sizeof invalid_arguments / sizeof invalid_arguments[0]);
  const int too_many_argument_count =
    (int)(sizeof too_many_arguments /
          sizeof too_many_arguments[0]);

  const CliCommand invalid_result = cli_parse_command(
    invalid_argument_count,
    invalid_arguments
  );
  const CliCommand too_many_result = cli_parse_command(
    too_many_argument_count,
    too_many_arguments
  );

  assert(invalid_result == CLI_COMMAND_INVALID);
  assert(too_many_result == CLI_COMMAND_INVALID);
}

int main(void)
{
  test_cli_parse_command_runs_without_command();
  test_cli_parse_command_accepts_help_commands();
  test_cli_parse_command_rejects_invalid_commands();

  puts("All CLI tests passed.");

  return 0;
}
