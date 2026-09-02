#pragma once

typedef enum CliCommand
{
  CLI_COMMAND_RUN,
  CLI_COMMAND_HELP,
  CLI_COMMAND_INVALID
} CliCommand;

CliCommand cli_parse_command(
    int argument_count,
    char *arguments[]
);

void cli_print_help(void);
