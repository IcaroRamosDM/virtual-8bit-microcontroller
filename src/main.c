#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "cli.h"
#include "cpu.h"

int main(int argument_count, char *arguments[])
{
  const CliCommand command = cli_parse_command(
    argument_count,
    arguments
  );

  switch (command)
  {
    case CLI_COMMAND_RUN:
      break;

    case CLI_COMMAND_HELP:
      cli_print_help();
      return EXIT_SUCCESS;

    case CLI_COMMAND_INVALID:
      fputs("Unknown command. Run 'make help'.\n", stderr);
      return EXIT_FAILURE;
  }

  const uint8_t program[] = {
    OPCODE_NOP,
    OPCODE_HALT
  };
  const size_t program_size = sizeof program / sizeof program[0];
  const uint64_t instruction_limit = CPU_MEMORY_SIZE;
  Cpu cpu = {0};

  const bool program_loaded = cpu_load_program(
    &cpu,
    program,
    program_size
  );

  if (!program_loaded)
  {
    fputs("Program does not fit in memory.\n", stderr);
    return EXIT_FAILURE;
  }

  const CpuRunResult result = cpu_run(
    &cpu,
    instruction_limit
  );

  puts("Virtual 8-bit microcontroller simulator");

  switch (result)
  {
    case CPU_RUN_HALTED:
      puts("Execution result: halted");
      break;

    case CPU_RUN_INVALID_OPCODE:
      fputs("Execution result: invalid opcode\n", stderr);
      return EXIT_FAILURE;

    case CPU_RUN_INSTRUCTION_LIMIT_REACHED:
      fputs(
        "Execution result: instruction limit reached\n",
        stderr
      );
      return EXIT_FAILURE;
  }

  printf("Program counter: %" PRIu8 "\n", cpu.program_counter);
  printf("Cycle count: %" PRIu64 "\n", cpu.cycle_count);

  return EXIT_SUCCESS;
}
