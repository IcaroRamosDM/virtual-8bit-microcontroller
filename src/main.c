#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "binary_reader.h"
#include "cli.h"
#include "cpu.h"
#include "program.h"
#include "cpu_trace.h"

int main(int argument_count, char *arguments[])
{
  const CliOptions options = cli_parse_arguments(
    argument_count,
    arguments
  );

  switch (options.command)
  {
    case CLI_COMMAND_RUN_DEMO:
    case CLI_COMMAND_RUN_BINARY:
      break;

    case CLI_COMMAND_HELP:
      cli_print_help();
      return EXIT_SUCCESS;

    case CLI_COMMAND_INVALID:
      fputs(
        "Invalid command or arguments. Run 'make help'.\n",
        stderr
      );
      return EXIT_FAILURE;
  }

  uint8_t binary_bytes[CPU_PROGRAM_MEMORY_SIZE] = {0};
  size_t binary_size = 0;
  Program program = program_get_demo();

  if (options.command == CLI_COMMAND_RUN_BINARY)
  {
    const bool binary_read_succeeded =
      binary_reader_read(
        options.binary_path,
        binary_bytes,
        sizeof binary_bytes,
        &binary_size
      );

    if (!binary_read_succeeded)
    {
      return EXIT_FAILURE;
    }

    if (binary_size == 0)
    {
      fprintf(
        stderr,
        "%s: binary program is empty\n",
        options.binary_path
      );

      return EXIT_FAILURE;
    }

    program = (Program){
      .bytes = binary_bytes,
      .size = binary_size
    };
  }

  const uint64_t instruction_limit = CPU_MEMORY_SIZE;
  Cpu cpu = {0};

  const bool program_loaded = cpu_load_program(
    &cpu,
    program.bytes,
    program.size
  );

  if (!program_loaded)
  {
    fputs("Program does not fit in program memory.\n", stderr);
    return EXIT_FAILURE;
  }

  cpu_set_input_port(
    &cpu,
    options.input_port_value
  );

  CpuStepObserver observer = NULL;
  void *observer_context = NULL;

  if (options.trace_enabled)
  {
    cpu_trace_print_header(stdout);
    observer = cpu_trace_observer;
    observer_context = stdout;
  }

  const CpuRunResult result =
    cpu_run_with_observer(
      &cpu,
      instruction_limit,
      observer,
      observer_context
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

    case CPU_RUN_STACK_OVERFLOW:
      fputs("Execution result: stack overflow\n", stderr);
      return EXIT_FAILURE;

    case CPU_RUN_STACK_UNDERFLOW:
      fputs("Execution result: stack underflow\n", stderr);
      return EXIT_FAILURE;
  }

  printf(
    "Register A: 0x%02X\n",
    (unsigned int)cpu.register_a
  );

  printf(
    "Register B: 0x%02X\n",
    (unsigned int)cpu.register_b
  );

  printf(
     "Stack pointer: 0x%02X\n",
     (unsigned int)cpu.stack_pointer
  );

  printf(
    "Input port: 0x%02X\n",
    (unsigned int)cpu_read_memory(
      &cpu,
      CPU_INPUT_PORT_ADDRESS
    )
  );

  printf(
    "Output port: 0x%02X\n",
    (unsigned int)cpu_get_output_port(&cpu)
  );

  printf(
    "Zero flag: %s\n",
    cpu.zero_flag ? "set" : "clear"
  );

  printf(
    "Carry flag: %s\n",
    cpu.carry_flag ? "set" : "clear"
  );

  printf(
    "Program counter: %" PRIu8 "\n",
    cpu.program_counter
  );

  printf(
    "Cycle count: %" PRIu64 "\n",
    cpu.cycle_count
  );

  return EXIT_SUCCESS;
}
