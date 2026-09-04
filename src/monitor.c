#include "monitor.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#include "binary_reader.h"
#include "byte_value.h"
#include "cpu_state.h"
#include "cpu_trace.h"

enum
{
  MONITOR_COMMAND_MAX_LENGTH = 255,
  MONITOR_LINE_BUFFER_SIZE =
    MONITOR_COMMAND_MAX_LENGTH + 2,
  MONITOR_INSTRUCTION_LIMIT = CPU_MEMORY_SIZE,
  MONITOR_MEMORY_BYTES_PER_ROW = 16,
  MONITOR_DEFAULT_MEMORY_COUNT =
    MONITOR_MEMORY_BYTES_PER_ROW
};

static const char HELP_COMMAND[] = "help";
static const char REGISTERS_COMMAND[] = "registers";
static const char STEP_COMMAND[] = "step";
static const char RUN_COMMAND[] = "run";
static const char RESET_COMMAND[] = "reset";
static const char INPUT_COMMAND[] = "input";
static const char MEMORY_COMMAND[] = "memory";
static const char LOAD_COMMAND[] = "load";
static const char BREAKPOINT_COMMAND[] = "breakpoint";
static const char TRACE_COMMAND[] = "trace";
static const char TRACE_ON_ARGUMENT[] = "on";
static const char TRACE_OFF_ARGUMENT[] = "off";
static const char BREAKPOINT_ADD_ARGUMENT[] = "add";
static const char BREAKPOINT_REMOVE_ARGUMENT[] = "remove";
static const char BREAKPOINT_LIST_ARGUMENT[] = "list";
static const char BREAKPOINT_CLEAR_ARGUMENT[] = "clear";
static const char QUIT_COMMAND[] = "quit";

static char *monitor_trim(char *text)
{
  while (isspace((unsigned char)text[0]))
  {
    ++text;
  }

  char *end = text + strlen(text);

  while (
    (end > text) &&
    isspace((unsigned char)end[-1])
  )
  {
    --end;
  }

  end[0] = '\0';

  return text;
}

static char *monitor_split_argument(char *command)
{
  char *cursor = command;

  while (
    (cursor[0] != '\0') &&
    !isspace((unsigned char)cursor[0])
  )
  {
    ++cursor;
  }

  if (cursor[0] == '\0')
  {
    return NULL;
  }

  cursor[0] = '\0';
  ++cursor;

  return monitor_trim(cursor);
}

static bool monitor_discard_remaining_line(FILE *input)
{
  int character = 0;

  do
  {
    character = fgetc(input);
  }
  while ((character != '\n') && (character != EOF));

  return ferror(input) == 0;
}

static bool monitor_reject_unexpected_argument(
    const char *argument,
    const char *usage,
    FILE *error_output
)
{
  if (argument == NULL)
  {
    return false;
  }

  fprintf(error_output, "Usage: %s\n", usage);

  return true;
}

static const char *monitor_step_result_name(
    CpuStepResult result
)
{
  switch (result)
  {
    case CPU_STEP_OK:
      return "ok";

    case CPU_STEP_HALTED:
      return "halted";

    case CPU_STEP_INVALID_OPCODE:
      return "invalid opcode";

    case CPU_STEP_STACK_OVERFLOW:
      return "stack overflow";

    case CPU_STEP_STACK_UNDERFLOW:
      return "stack underflow";
  }

  return "unknown";
}

static void monitor_print_help(FILE *output)
{
  fputs(
    "Monitor commands:\n"
    "  help                         Show this command list.\n"
    "  registers                    Show the current CPU state.\n"
    "  step                         Execute one instruction.\n"
    "  run                          Run until HALT, an error, or the limit.\n"
    "  reset                        Reset and reload the current program.\n"
    "  input <byte>                 Set the virtual input port.\n"
    "  memory <address> [count]     Show memory bytes.\n"
    "  load <program.bin>           Load a binary and reset the CPU.\n"
    "  trace                        Show the trace status.\n"
    "  trace on|off                 Enable or disable tracing.\n"
    "  breakpoint add <address>     Add a breakpoint.\n"
    "  breakpoint remove <address>  Remove a breakpoint.\n"
    "  breakpoint list              List all breakpoints.\n"
    "  breakpoint clear             Remove all breakpoints.\n"
    "  quit                         Leave the monitor.\n",
    output
  );
}

static void monitor_execute_step(
    Cpu *cpu,
    bool trace_enabled,
    FILE *output
)
{
  const bool instruction_was_attempted = !cpu->halted;
  const uint8_t instruction_address =
    cpu->program_counter;

  const uint8_t opcode = cpu_read_memory(
    cpu,
    instruction_address
  );

  const CpuStepResult result = cpu_step(cpu);

  if (trace_enabled && instruction_was_attempted)
  {
    cpu_trace_print_header(output);

    cpu_trace_observer(
      instruction_address,
      opcode,
      cpu,
      result,
      output
    );
  }

  fprintf(
    output,
    "Step result: %s\n",
    monitor_step_result_name(result)
  );

  cpu_state_print(output, cpu);
}

static void monitor_print_breakpoint_stop(
    const Cpu *cpu,
    FILE *output
)
{
  fprintf(
    output,
    "Breakpoint reached at 0x%02X.\n",
    (unsigned int)cpu->program_counter
  );

  cpu_state_print(output, cpu);
}

static void monitor_execute_run(
    Cpu *cpu,
    const bool breakpoints[CPU_MEMORY_SIZE],
    bool trace_enabled,
    FILE *output
)
{
  if (cpu->halted)
  {
    fputs("Execution result: halted\n", output);
    cpu_state_print(output, cpu);
    return;
  }

  if (breakpoints[cpu->program_counter])
  {
    monitor_print_breakpoint_stop(cpu, output);
    return;
  }

  if (trace_enabled)
  {
    cpu_trace_print_header(output);
  }

  for (
    uint64_t executed_instructions = 0;
    executed_instructions < MONITOR_INSTRUCTION_LIMIT;
    ++executed_instructions
  )
  {
    const uint8_t instruction_address =
      cpu->program_counter;

    const uint8_t opcode = cpu_read_memory(
      cpu,
      instruction_address
    );

    const CpuStepResult result = cpu_step(cpu);

    if (trace_enabled)
    {
      cpu_trace_observer(
        instruction_address,
        opcode,
        cpu,
        result,
        output
      );
    }

    if (result != CPU_STEP_OK)
    {
      fprintf(
        output,
        "Execution result: %s\n",
        monitor_step_result_name(result)
      );

      cpu_state_print(output, cpu);
      return;
    }

    if (breakpoints[cpu->program_counter])
    {
      monitor_print_breakpoint_stop(cpu, output);
      return;
    }
  }

  fputs(
    "Execution result: instruction limit reached\n",
    output
  );

  cpu_state_print(output, cpu);
}

static bool monitor_reset_cpu(
    Cpu *cpu,
    Program program,
    FILE *output,
    FILE *error_output
)
{
  cpu_reset(cpu);

  const bool loaded = cpu_load_program(
    cpu,
    program.bytes,
    program.size
  );

  if (!loaded)
  {
    fputs(
      "Monitor could not reload the program.\n",
      error_output
    );

    return false;
  }

  fputs("CPU reset and program reloaded.\n", output);

  return true;
}

static void monitor_set_input(
    Cpu *cpu,
    const char *argument,
    FILE *output,
    FILE *error_output
)
{
  uint8_t value = 0;

  if (
    (argument == NULL) ||
    !byte_value_parse(argument, &value)
  )
  {
    fputs(
      "Usage: input <byte>\n"
      "The byte must be decimal or 0x-prefixed hexadecimal.\n",
      error_output
    );

    return;
  }

  cpu_set_input_port(cpu, value);

  fprintf(
    output,
    "Input port set to 0x%02X.\n",
    (unsigned int)value
  );
}

static void monitor_print_memory(
    Cpu *cpu,
    char *argument,
    FILE *output,
    FILE *error_output
)
{
  if (argument == NULL)
  {
    fputs(
      "Usage: memory <address> [count]\n",
      error_output
    );

    return;
  }

  char *const count_text =
    monitor_split_argument(argument);

  uint8_t start_address = 0;

  if (!byte_value_parse(argument, &start_address))
  {
    fputs(
      "Usage: memory <address> [count]\n"
      "The address must be an 8-bit value.\n",
      error_output
    );

    return;
  }

  size_t byte_count = MONITOR_DEFAULT_MEMORY_COUNT;

  if (count_text != NULL)
  {
    uint8_t parsed_count = 0;

    if (
      !byte_value_parse(count_text, &parsed_count) ||
      (parsed_count == 0)
    )
    {
      fputs(
        "Usage: memory <address> [count]\n"
        "The count must be from 1 through 255.\n",
        error_output
      );

      return;
    }

    byte_count = parsed_count;
  }

  const size_t start = start_address;
  const size_t available_byte_count =
    CPU_MEMORY_SIZE - start;

  if (byte_count > available_byte_count)
  {
    byte_count = available_byte_count;
  }

  for (size_t offset = 0; offset < byte_count; ++offset)
  {
    const size_t current_address = start + offset;

    if ((offset % MONITOR_MEMORY_BYTES_PER_ROW) == 0)
    {
      if (offset != 0)
      {
        fputc('\n', output);
      }

      fprintf(
        output,
        "0x%02X:",
        (unsigned int)current_address
      );
    }

    fprintf(
      output,
      " %02X",
      (unsigned int)cpu_read_memory(
        cpu,
        (uint8_t)current_address
      )
    );
  }

  fputc('\n', output);
}

static void monitor_load_program(
    Cpu *cpu,
    Program *current_program,
    uint8_t current_program_bytes[
      CPU_PROGRAM_MEMORY_SIZE
    ],
    const char *binary_path,
    FILE *output,
    FILE *error_output
)
{
  if ((binary_path == NULL) || (binary_path[0] == '\0'))
  {
    fputs("Usage: load <program.bin>\n", error_output);
    return;
  }

  uint8_t candidate_bytes[CPU_PROGRAM_MEMORY_SIZE] = {0};
  size_t candidate_size = 0;

  const bool read_succeeded = binary_reader_read(
    binary_path,
    candidate_bytes,
    sizeof candidate_bytes,
    &candidate_size
  );

  if (!read_succeeded)
  {
    return;
  }

  if (candidate_size == 0)
  {
    fprintf(
      error_output,
      "%s: binary program is empty\n",
      binary_path
    );

    return;
  }

  cpu_reset(cpu);

  const bool loaded = cpu_load_program(
    cpu,
    candidate_bytes,
    candidate_size
  );

  if (!loaded)
  {
    fputs(
      "Monitor could not load the new program.\n",
      error_output
    );

    return;
  }

  memcpy(
    current_program_bytes,
    candidate_bytes,
    candidate_size
  );

  *current_program = (Program){
    .bytes = current_program_bytes,
    .size = candidate_size
  };

  fprintf(
    output,
    "Loaded %zu byte(s) from '%s'; CPU reset.\n",
    candidate_size,
    binary_path
  );
}

static void monitor_manage_trace(
    const char *argument,
    bool *trace_enabled,
    FILE *output,
    FILE *error_output
)
{
  if (argument == NULL)
  {
    fprintf(
      output,
      "Trace is %s.\n",
      *trace_enabled ? "on" : "off"
    );

    return;
  }

  if (strcmp(argument, TRACE_ON_ARGUMENT) == 0)
  {
    *trace_enabled = true;
    fputs("Trace enabled.\n", output);
    return;
  }

  if (strcmp(argument, TRACE_OFF_ARGUMENT) == 0)
  {
    *trace_enabled = false;
    fputs("Trace disabled.\n", output);
    return;
  }

  fputs("Usage: trace [on|off]\n", error_output);
}

static void monitor_print_breakpoint_usage(
    FILE *error_output
)
{
  fputs(
    "Usage:\n"
    "  breakpoint add <address>\n"
    "  breakpoint remove <address>\n"
    "  breakpoint list\n"
    "  breakpoint clear\n",
    error_output
  );
}

static void monitor_list_breakpoints(
    const bool breakpoints[CPU_MEMORY_SIZE],
    FILE *output
)
{
  bool found = false;

  for (size_t address = 0; address < CPU_MEMORY_SIZE; ++address)
  {
    if (breakpoints[address])
    {
      if (!found)
      {
        fputs("Breakpoints:\n", output);
        found = true;
      }

      fprintf(
        output,
        "  0x%02X\n",
        (unsigned int)address
      );
    }
  }

  if (!found)
  {
    fputs("No breakpoints set.\n", output);
  }
}

static void monitor_clear_breakpoints(
    bool breakpoints[CPU_MEMORY_SIZE],
    FILE *output
)
{
  for (size_t address = 0; address < CPU_MEMORY_SIZE; ++address)
  {
    breakpoints[address] = false;
  }

  fputs("All breakpoints cleared.\n", output);
}

static void monitor_manage_breakpoint(
    char *argument,
    bool breakpoints[CPU_MEMORY_SIZE],
    FILE *output,
    FILE *error_output
)
{
  if (argument == NULL)
  {
    monitor_print_breakpoint_usage(error_output);
    return;
  }

  char *const address_text =
    monitor_split_argument(argument);

  if (strcmp(argument, BREAKPOINT_LIST_ARGUMENT) == 0)
  {
    if (address_text != NULL)
    {
      monitor_print_breakpoint_usage(error_output);
      return;
    }

    monitor_list_breakpoints(breakpoints, output);
    return;
  }

  if (strcmp(argument, BREAKPOINT_CLEAR_ARGUMENT) == 0)
  {
    if (address_text != NULL)
    {
      monitor_print_breakpoint_usage(error_output);
      return;
    }

    monitor_clear_breakpoints(breakpoints, output);
    return;
  }

  if (address_text == NULL)
  {
    monitor_print_breakpoint_usage(error_output);
    return;
  }

  uint8_t address = 0;

  if (!byte_value_parse(address_text, &address))
  {
    fputs(
      "Breakpoint address must be an 8-bit value.\n",
      error_output
    );

    return;
  }

  if (strcmp(argument, BREAKPOINT_ADD_ARGUMENT) == 0)
  {
    if (breakpoints[address])
    {
      fprintf(
        output,
        "Breakpoint already set at 0x%02X.\n",
        (unsigned int)address
      );

      return;
    }

    breakpoints[address] = true;

    fprintf(
      output,
      "Breakpoint added at 0x%02X.\n",
      (unsigned int)address
    );

    return;
  }

  if (strcmp(argument, BREAKPOINT_REMOVE_ARGUMENT) == 0)
  {
    if (!breakpoints[address])
    {
      fprintf(
        output,
        "No breakpoint set at 0x%02X.\n",
        (unsigned int)address
      );

      return;
    }

    breakpoints[address] = false;

    fprintf(
      output,
      "Breakpoint removed from 0x%02X.\n",
      (unsigned int)address
    );

    return;
  }

  monitor_print_breakpoint_usage(error_output);
}

bool monitor_run(
    Cpu *cpu,
    Program program,
    FILE *input,
    FILE *output,
    FILE *error_output
)
{
  if (
    (cpu == NULL) ||
    (input == NULL) ||
    (output == NULL) ||
    (error_output == NULL) ||
    ((program.bytes == NULL) && (program.size != 0)) ||
    (program.size > CPU_PROGRAM_MEMORY_SIZE)
  )
  {
    return false;
  }

  uint8_t current_program_bytes[
    CPU_PROGRAM_MEMORY_SIZE
  ] = {0};

  if (program.size != 0)
  {
    memcpy(
        current_program_bytes,
        program.bytes,
        program.size
    );
  }

  Program current_program =
  {
    .bytes = current_program_bytes,
    .size = program.size
  };

  bool trace_enabled = false;
  bool breakpoints[CPU_MEMORY_SIZE] = {false};

  fputs("VM8 interactive monitor\n", output);
  fputs("Type 'help' to list the available commands.\n", output);

  char line[MONITOR_LINE_BUFFER_SIZE] = {0};

  for (;;)
  {
    fputs("vm8> ", output);
    fflush(output);

    if (fgets(line, sizeof line, input) == NULL)
    {
      if (feof(input))
      {
        fputc('\n', output);
        return true;
      }

      fputs("Monitor input error.\n", error_output);
      return false;
    }

    if (
      (strchr(line, '\n') == NULL) &&
      !feof(input)
    )
    {
      if (!monitor_discard_remaining_line(input))
      {
        fputs("Monitor input error.\n", error_output);
        return false;
      }

      fprintf(
        error_output,
        "Monitor command exceeds %u characters.\n",
        (unsigned int)MONITOR_COMMAND_MAX_LENGTH
      );

      continue;
    }

    line[strcspn(line, "\r\n")] = '\0';

    char *const command = monitor_trim(line);

    if (command[0] == '\0')
    {
      continue;
    }

    char *const argument =
      monitor_split_argument(command);

    if (strcmp(command, HELP_COMMAND) == 0)
    {
      if (
        monitor_reject_unexpected_argument(
          argument,
          HELP_COMMAND,
          error_output
        )
      )
      {
        continue;
      }

      monitor_print_help(output);
      continue;
    }

    if (strcmp(command, REGISTERS_COMMAND) == 0)
    {
      if (
        monitor_reject_unexpected_argument(
          argument,
          REGISTERS_COMMAND,
          error_output
        )
      )
      {
        continue;
      }

      cpu_state_print(output, cpu);
      continue;
    }

    if (strcmp(command, STEP_COMMAND) == 0)
    {
      if (
        monitor_reject_unexpected_argument(
          argument,
          STEP_COMMAND,
          error_output
        )
      )
      {
        continue;
      }

      monitor_execute_step(
          cpu,
          trace_enabled,
          output
      );
      continue;
    }

    if (strcmp(command, RUN_COMMAND) == 0)
    {
      if (
        monitor_reject_unexpected_argument(
          argument,
          RUN_COMMAND,
          error_output
        )
      )
      {
        continue;
      }

      monitor_execute_run(
          cpu,
          breakpoints,
          trace_enabled,
          output
      );
      continue;
    }

    if (strcmp(command, RESET_COMMAND) == 0)
    {
      if (
        monitor_reject_unexpected_argument(
          argument,
          RESET_COMMAND,
          error_output
        )
      )
      {
        continue;
      }

      if (
        !monitor_reset_cpu(
          cpu,
          current_program,
          output,
          error_output
        )
      )
      {
        return false;
      }

      continue;
    }

    if (strcmp(command, INPUT_COMMAND) == 0)
    {
      monitor_set_input(
        cpu,
        argument,
        output,
        error_output
      );

      continue;
    }

    if (strcmp(command, MEMORY_COMMAND) == 0)
    {
      monitor_print_memory(
        cpu,
        argument,
        output,
        error_output
      );

      continue;
    }

    if (strcmp(command, LOAD_COMMAND) == 0)
    {
      monitor_load_program(
        cpu,
        &current_program,
        current_program_bytes,
        argument,
        output,
        error_output
      );

      continue;
    }

    if (strcmp(command, TRACE_COMMAND) == 0)
    {
      monitor_manage_trace(
        argument,
        &trace_enabled,
        output,
        error_output
      );

      continue;
    }

    if (strcmp(command, BREAKPOINT_COMMAND) == 0)
    {
      monitor_manage_breakpoint(
        argument,
        breakpoints,
        output,
        error_output
      );

      continue;
    }

    if (strcmp(command, QUIT_COMMAND) == 0)
    {
      if (
        monitor_reject_unexpected_argument(
          argument,
          QUIT_COMMAND,
          error_output
        )
      )
      {
        continue;
      }

      fputs("Leaving VM8 monitor.\n", output);
      return true;
    }

    fprintf(
      error_output,
      "Unknown monitor command: '%s'. Type 'help'.\n",
      command
    );
  }
}
