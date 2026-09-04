#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli.h"

enum
{
  TEST_INPUT_VALUE = 0xA5
};

static CliOptions parse_no_arguments(void)
{
  char program_name[] = "vm8";

  char *arguments[] =
  {
    program_name
  };

  const int argument_count =
    (int)(sizeof arguments / sizeof arguments[0]);

  return cli_parse_arguments(
    argument_count,
    arguments
  );
}

static CliOptions parse_one_argument(char *argument)
{
  char program_name[] = "vm8";

  char *arguments[] =
  {
    program_name,
    argument
  };

  const int argument_count =
    (int)(sizeof arguments / sizeof arguments[0]);

  return cli_parse_arguments(
    argument_count,
    arguments
  );
}

static CliOptions parse_two_arguments(
    char *first_argument,
    char *second_argument
)
{
  char program_name[] = "vm8";

  char *arguments[] =
  {
    program_name,
    first_argument,
    second_argument
  };

  const int argument_count =
    (int)(sizeof arguments / sizeof arguments[0]);

  return cli_parse_arguments(
    argument_count,
    arguments
  );
}

static CliOptions parse_three_arguments(
    char *first_argument,
    char *second_argument,
    char *third_argument
)
{
  char program_name[] = "vm8";

  char *arguments[] =
  {
    program_name,
    first_argument,
    second_argument,
    third_argument
  };

  const int argument_count =
    (int)(sizeof arguments / sizeof arguments[0]);

  return cli_parse_arguments(
    argument_count,
    arguments
  );
}

static CliOptions parse_four_arguments(
    char *first_argument,
    char *second_argument,
    char *third_argument,
    char *fourth_argument
)
{
  char program_name[] = "vm8";

  char *arguments[] =
  {
    program_name,
    first_argument,
    second_argument,
    third_argument,
    fourth_argument
  };

  const int argument_count =
    (int)(sizeof arguments / sizeof arguments[0]);

  return cli_parse_arguments(
    argument_count,
    arguments
  );
}

static void assert_zero_input(const CliOptions *options)
{
  assert(options->input_port_value == 0);
}

static void assert_invalid_options(const CliOptions *options)
{
  assert(options->command == CLI_COMMAND_INVALID);
  assert(options->binary_path == NULL);
  assert(!options->trace_enabled);
  assert_zero_input(options);
}

static void test_runs_demo_without_command(void)
{
  const CliOptions options = parse_no_arguments();

  assert(options.command == CLI_COMMAND_RUN_DEMO);
  assert(options.binary_path == NULL);
  assert(!options.trace_enabled);
  assert_zero_input(&options);
}

static void test_accepts_help_commands(void)
{
  char help_command[] = "help";
  char long_help_command[] = "--help";

  const CliOptions help_options =
    parse_one_argument(help_command);

  const CliOptions long_help_options =
    parse_one_argument(long_help_command);

  assert(help_options.command == CLI_COMMAND_HELP);
  assert(help_options.binary_path == NULL);
  assert(!help_options.trace_enabled);
  assert_zero_input(&help_options);

  assert(long_help_options.command == CLI_COMMAND_HELP);
  assert(long_help_options.binary_path == NULL);
  assert(!long_help_options.trace_enabled);
  assert_zero_input(&long_help_options);
}

static void test_accepts_run_alias(void)
{
  char run_command[] = "run";

  const CliOptions options =
    parse_one_argument(run_command);

  assert(options.command == CLI_COMMAND_RUN_DEMO);
  assert(options.binary_path == NULL);
  assert(!options.trace_enabled);
  assert_zero_input(&options);
}

static void test_accepts_binary_program_path(void)
{
  char run_command[] = "run";
  char binary_path[] = "build/demo.bin";

  const CliOptions options =
    parse_two_arguments(
      run_command,
      binary_path
    );

  assert(options.command == CLI_COMMAND_RUN_BINARY);
  assert(options.binary_path != NULL);
  assert(strcmp(options.binary_path, binary_path) == 0);
  assert(!options.trace_enabled);
  assert_zero_input(&options);
}

static void test_accepts_trace_modes(void)
{
  char trace_command[] = "trace";
  char binary_path[] = "build/demo.bin";

  const CliOptions built_in_options =
    parse_one_argument(trace_command);

  const CliOptions binary_options =
    parse_two_arguments(
      trace_command,
      binary_path
    );

  assert(
    built_in_options.command ==
    CLI_COMMAND_RUN_DEMO
  );

  assert(built_in_options.binary_path == NULL);
  assert(built_in_options.trace_enabled);
  assert_zero_input(&built_in_options);

  assert(
    binary_options.command ==
    CLI_COMMAND_RUN_BINARY
  );

  assert(binary_options.binary_path != NULL);

  assert(
    strcmp(binary_options.binary_path, binary_path) ==
    0
  );

  assert(binary_options.trace_enabled);
  assert_zero_input(&binary_options);
}

static void test_accepts_monitor_modes(void)
{
  char monitor_command[] = "monitor";
  char binary_path[] = "build/demo.bin";

  const CliOptions built_in_options =
    parse_one_argument(monitor_command);

  const CliOptions binary_options =
    parse_two_arguments(
      monitor_command,
      binary_path
    );

  assert(
    built_in_options.command ==
    CLI_COMMAND_MONITOR_DEMO
  );

  assert(built_in_options.binary_path == NULL);
  assert(!built_in_options.trace_enabled);
  assert_zero_input(&built_in_options);

  assert(
    binary_options.command ==
    CLI_COMMAND_MONITOR_BINARY
  );

  assert(binary_options.binary_path != NULL);

  assert(
    strcmp(binary_options.binary_path, binary_path) ==
    0
  );

  assert(!binary_options.trace_enabled);
  assert_zero_input(&binary_options);
}

static void test_rejects_invalid_monitor_layouts(void)
{
  char monitor_command[] = "monitor";
  char input_option[] = "--input";
  char binary_path[] = "build/demo.bin";
  char extra_argument[] = "extra";

  const CliOptions input_options =
    parse_two_arguments(
      monitor_command,
      input_option
    );

  const CliOptions extra_options =
    parse_three_arguments(
      monitor_command,
      binary_path,
      extra_argument
    );

  assert_invalid_options(&input_options);
  assert_invalid_options(&extra_options);
}

static void test_accepts_input_values(void)
{
  char run_command[] = "run";
  char trace_command[] = "trace";
  char input_option[] = "--input";
  char decimal_value[] = "165";
  char hexadecimal_value[] = "0xA5";
  char zero_value[] = "0";
  char maximum_value[] = "0xFF";
  char binary_path[] = "build/demo.bin";

  const CliOptions decimal_demo_options =
    parse_three_arguments(
      run_command,
      input_option,
      decimal_value
    );

  const CliOptions hexadecimal_trace_options =
    parse_three_arguments(
      trace_command,
      input_option,
      hexadecimal_value
    );

  const CliOptions binary_zero_options =
    parse_four_arguments(
      run_command,
      binary_path,
      input_option,
      zero_value
    );

  const CliOptions binary_maximum_options =
    parse_four_arguments(
      trace_command,
      binary_path,
      input_option,
      maximum_value
    );

  assert(
    decimal_demo_options.command ==
    CLI_COMMAND_RUN_DEMO
  );

  assert(decimal_demo_options.binary_path == NULL);
  assert(!decimal_demo_options.trace_enabled);

  assert(
    decimal_demo_options.input_port_value ==
    TEST_INPUT_VALUE
  );

  assert(
    hexadecimal_trace_options.command ==
    CLI_COMMAND_RUN_DEMO
  );

  assert(hexadecimal_trace_options.binary_path == NULL);
  assert(hexadecimal_trace_options.trace_enabled);

  assert(
    hexadecimal_trace_options.input_port_value ==
    TEST_INPUT_VALUE
  );

  assert(
    binary_zero_options.command ==
    CLI_COMMAND_RUN_BINARY
  );

  assert(
    strcmp(binary_zero_options.binary_path, binary_path) ==
    0
  );

  assert(!binary_zero_options.trace_enabled);
  assert_zero_input(&binary_zero_options);

  assert(
    binary_maximum_options.command ==
    CLI_COMMAND_RUN_BINARY
  );

  assert(
    strcmp(
      binary_maximum_options.binary_path,
      binary_path
    ) ==
    0
  );

  assert(binary_maximum_options.trace_enabled);

  assert(
    binary_maximum_options.input_port_value ==
    UINT8_MAX
  );
}

static void test_rejects_invalid_input_values(void)
{
  char run_command[] = "run";
  char input_option[] = "--input";
  char empty_value[] = "";
  char negative_value[] = "-1";
  char signed_value[] = "+1";
  char leading_space_value[] = " 1";
  char oversized_value[] = "256";
  char incomplete_hexadecimal_value[] = "0x";
  char trailing_text_value[] = "12abc";

  const CliOptions empty_options =
    parse_three_arguments(
      run_command,
      input_option,
      empty_value
    );

  const CliOptions negative_options =
    parse_three_arguments(
      run_command,
      input_option,
      negative_value
    );

  const CliOptions signed_options =
    parse_three_arguments(
      run_command,
      input_option,
      signed_value
    );

  const CliOptions leading_space_options =
    parse_three_arguments(
      run_command,
      input_option,
      leading_space_value
    );

  const CliOptions oversized_options =
    parse_three_arguments(
      run_command,
      input_option,
      oversized_value
    );

  const CliOptions incomplete_hexadecimal_options =
    parse_three_arguments(
      run_command,
      input_option,
      incomplete_hexadecimal_value
    );

  const CliOptions trailing_text_options =
    parse_three_arguments(
      run_command,
      input_option,
      trailing_text_value
    );

  assert_invalid_options(&empty_options);
  assert_invalid_options(&negative_options);
  assert_invalid_options(&signed_options);
  assert_invalid_options(&leading_space_options);
  assert_invalid_options(&oversized_options);
  assert_invalid_options(&incomplete_hexadecimal_options);
  assert_invalid_options(&trailing_text_options);
}

static void test_rejects_invalid_argument_layouts(void)
{
  char run_command[] = "run";
  char help_command[] = "help";
  char unknown_command[] = "unknown";
  char input_option[] = "--input";
  char input_value[] = "165";
  char binary_path[] = "build/demo.bin";
  char extra_argument[] = "extra";

  const CliOptions unknown_command_options =
    parse_one_argument(unknown_command);

  const CliOptions missing_input_options =
    parse_two_arguments(
      run_command,
      input_option
    );

  const CliOptions unexpected_extra_options =
    parse_three_arguments(
      run_command,
      binary_path,
      extra_argument
    );

  const CliOptions misplaced_input_options =
    parse_four_arguments(
      run_command,
      input_option,
      input_value,
      binary_path
    );

  const CliOptions help_input_options =
    parse_three_arguments(
      help_command,
      input_option,
      input_value
    );

  assert_invalid_options(&unknown_command_options);
  assert_invalid_options(&missing_input_options);
  assert_invalid_options(&unexpected_extra_options);
  assert_invalid_options(&misplaced_input_options);
  assert_invalid_options(&help_input_options);
}

int main(void)
{
  test_runs_demo_without_command();
  test_accepts_help_commands();
  test_accepts_run_alias();
  test_accepts_binary_program_path();
  test_accepts_trace_modes();
  test_accepts_monitor_modes();
  test_rejects_invalid_monitor_layouts();
  test_accepts_input_values();
  test_rejects_invalid_input_values();
  test_rejects_invalid_argument_layouts();

  puts("All CLI tests passed.");

  return EXIT_SUCCESS;
}
