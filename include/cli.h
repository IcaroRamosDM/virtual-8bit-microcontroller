#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef enum CliCommand
{
  CLI_COMMAND_RUN_DEMO,
  CLI_COMMAND_RUN_BINARY,
  CLI_COMMAND_MONITOR_DEMO,
  CLI_COMMAND_MONITOR_BINARY,
  CLI_COMMAND_HELP,
  CLI_COMMAND_INVALID
} CliCommand;

typedef struct CliOptions
{
  CliCommand command;
  const char *binary_path;
  bool trace_enabled;
  uint8_t input_port_value;
} CliOptions;

CliOptions cli_parse_arguments(
    int argument_count,
    char *arguments[]
);

void cli_print_help(void);
void cli_write_help(FILE *stream);
