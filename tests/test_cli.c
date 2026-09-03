#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli.h"

static void test_runs_demo_without_command(void)
{
  char program_name[] = "vm8";
  char *arguments[] =
  {
    program_name
  };

  const int argument_count =
    (int)(sizeof arguments / sizeof arguments[0]);

  const CliOptions options = cli_parse_arguments(
    argument_count,
    arguments
  );

  assert(options.command == CLI_COMMAND_RUN_DEMO);
  assert(options.binary_path == NULL);
}

static void test_accepts_help_commands(void)
{
  char program_name[] = "vm8";
  char help_command[] = "help";
  char long_help_command[] = "--help";

  char *help_arguments[] =
  {
    program_name,
    help_command
  };

  char *long_help_arguments[] =
  {
    program_name,
    long_help_command
  };

  const int help_argument_count =
    (int)(
      sizeof help_arguments /
      sizeof help_arguments[0]
    );

  const int long_help_argument_count =
    (int)(
      sizeof long_help_arguments /
      sizeof long_help_arguments[0]
    );

  const CliOptions help_options = cli_parse_arguments(
    help_argument_count,
    help_arguments
  );

  const CliOptions long_help_options =
    cli_parse_arguments(
      long_help_argument_count,
      long_help_arguments
    );

  assert(help_options.command == CLI_COMMAND_HELP);
  assert(help_options.binary_path == NULL);
  assert(long_help_options.command == CLI_COMMAND_HELP);
  assert(long_help_options.binary_path == NULL);
}

static void test_accepts_binary_program_path(void)
{
  char program_name[] = "vm8";
  char run_command[] = "run";
  char binary_path[] = "build/demo.bin";

  char *arguments[] =
  {
    program_name,
    run_command,
    binary_path
  };

  const int argument_count =
    (int)(sizeof arguments / sizeof arguments[0]);

  const CliOptions options = cli_parse_arguments(
    argument_count,
    arguments
  );

  assert(options.command == CLI_COMMAND_RUN_BINARY);
  assert(options.binary_path != NULL);
  assert(strcmp(options.binary_path, binary_path) == 0);
}

static void test_rejects_invalid_arguments(void)
{
  char program_name[] = "vm8";
  char run_command[] = "run";
  char unknown_command[] = "unknown";
  char binary_path[] = "build/demo.bin";
  char extra_argument[] = "extra";

  char *missing_path_arguments[] =
  {
    program_name,
    run_command
  };

  char *unknown_command_arguments[] =
  {
    program_name,
    unknown_command
  };

  char *extra_arguments[] =
  {
    program_name,
    run_command,
    binary_path,
    extra_argument
  };

  const int missing_path_argument_count =
    (int)(
      sizeof missing_path_arguments /
      sizeof missing_path_arguments[0]
    );

  const int unknown_command_argument_count =
    (int)(
      sizeof unknown_command_arguments /
      sizeof unknown_command_arguments[0]
    );

  const int extra_argument_count =
    (int)(
      sizeof extra_arguments /
      sizeof extra_arguments[0]
    );

  const CliOptions missing_path_options =
    cli_parse_arguments(
      missing_path_argument_count,
      missing_path_arguments
    );

  const CliOptions unknown_command_options =
    cli_parse_arguments(
      unknown_command_argument_count,
      unknown_command_arguments
    );

  const CliOptions extra_options = cli_parse_arguments(
    extra_argument_count,
    extra_arguments
  );

  assert(
    missing_path_options.command ==
    CLI_COMMAND_INVALID
  );

  assert(
    unknown_command_options.command ==
    CLI_COMMAND_INVALID
  );

  assert(extra_options.command == CLI_COMMAND_INVALID);
}

int main(void)
{
  test_runs_demo_without_command();
  test_accepts_help_commands();
  test_accepts_binary_program_path();
  test_rejects_invalid_arguments();

  puts("All CLI tests passed.");

  return EXIT_SUCCESS;
}
