#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"
#include "monitor.h"
#include "test_stream.h"

enum
{
  TEST_STREAM_CAPACITY = 4096,
  TEST_COMMAND_BUFFER_CAPACITY = 512,
  MONITOR_TEST_MAX_COMMAND_LENGTH = 255,
  MONITOR_OVERLONG_COMMAND_LENGTH =
    MONITOR_TEST_MAX_COMMAND_LENGTH + 1,
  TEST_LOADED_VALUE = 0x2A,
  TEST_REPLACEMENT_VALUE = 0x5A,
  TEST_INPUT_VALUE = 0xA5,
  TEST_OUTPUT_VALUE = 0x5A,
  TEST_BREAKPOINT_ADDRESS = 0x04
};

static const char TEST_BINARY_PATH[] =
  "build/test_monitor_program.bin";

static const char TEST_EMPTY_BINARY_PATH[] =
  "build/test_monitor_empty.bin";

static const uint8_t TEST_PROGRAM_BYTES[] =
{
  OPCODE_LOAD_IMMEDIATE_A,
  TEST_LOADED_VALUE,
  OPCODE_HALT
};

static const uint8_t TEST_REPLACEMENT_PROGRAM_BYTES[] =
{
  OPCODE_LOAD_IMMEDIATE_B,
  TEST_REPLACEMENT_VALUE,
  OPCODE_HALT
};

static const uint8_t TEST_BREAKPOINT_PROGRAM_BYTES[] =
{
  OPCODE_LOAD_IMMEDIATE_A,
  TEST_LOADED_VALUE,
  OPCODE_LOAD_IMMEDIATE_B,
  TEST_LOADED_VALUE,
  OPCODE_ADD_A_B,
  OPCODE_HALT
};

static Program get_test_program(void)
{
  return (Program){
    .bytes = TEST_PROGRAM_BYTES,
    .size = sizeof TEST_PROGRAM_BYTES
  };
}

static Program get_breakpoint_test_program(void)
{
  return (Program){
    .bytes = TEST_BREAKPOINT_PROGRAM_BYTES,
    .size = sizeof TEST_BREAKPOINT_PROGRAM_BYTES
  };
}

static FILE *create_input_stream(const char *contents)
{
  FILE *const input = test_tmpfile();

  assert(input != NULL);

  if (contents != NULL)
  {
    const int write_result = fputs(contents, input);

    assert(write_result >= 0);
  }

  const int flush_result = fflush(input);

  assert(flush_result == 0);

  rewind(input);

  return input;
}

static void assert_stream_equals(
    FILE *stream,
    const char *expected_contents
)
{
  const int flush_result = fflush(stream);

  assert(flush_result == 0);

  rewind(stream);

  char actual_contents[TEST_STREAM_CAPACITY] = {0};

  const size_t read_byte_count = fread(
    actual_contents,
    sizeof actual_contents[0],
    sizeof actual_contents - 1,
    stream
  );

  const size_t expected_byte_count =
    strlen(expected_contents);

  assert(ferror(stream) == 0);
  assert(read_byte_count == expected_byte_count);
  assert(strcmp(actual_contents, expected_contents) == 0);
}

static void assert_stream_contains(
    FILE *stream,
    const char *expected_fragment
)
{
  const int flush_result = fflush(stream);

  assert(flush_result == 0);

  rewind(stream);

  char actual_contents[TEST_STREAM_CAPACITY] = {0};

  const size_t read_byte_count = fread(
    actual_contents,
    sizeof actual_contents[0],
    sizeof actual_contents - 1,
    stream
  );

  assert(ferror(stream) == 0);
  assert(read_byte_count < sizeof actual_contents);
  assert(strstr(actual_contents, expected_fragment) != NULL);
}

static void assert_stream_fragment_count(
    FILE *stream,
    const char *expected_fragment,
    size_t expected_count
)
{
  const int flush_result = fflush(stream);

  assert(flush_result == 0);
  assert(expected_fragment[0] != '\0');

  rewind(stream);

  char actual_contents[TEST_STREAM_CAPACITY] = {0};

  const size_t read_byte_count = fread(
    actual_contents,
    sizeof actual_contents[0],
    sizeof actual_contents - 1,
    stream
  );

  assert(ferror(stream) == 0);
  assert(read_byte_count < sizeof actual_contents);

  size_t actual_count = 0;
  const char *search_position = actual_contents;
  const size_t fragment_length = strlen(expected_fragment);

  while (
    (search_position = strstr(
      search_position,
      expected_fragment
    )) != NULL
  )
  {
    ++actual_count;
    search_position += fragment_length;
  }

  assert(actual_count == expected_count);
}

static void write_binary_file(
    const char *path,
    const uint8_t *bytes,
    size_t byte_count
)
{
  FILE *const binary_file = fopen(path, "wb");

  assert(binary_file != NULL);

  if (byte_count != 0)
  {
    const size_t written_byte_count = fwrite(
      bytes,
      sizeof bytes[0],
      byte_count,
      binary_file
    );

    assert(written_byte_count == byte_count);
  }

  const int close_result = fclose(binary_file);

  assert(close_result == 0);
}

static void close_stream(FILE *stream)
{
  const int close_result = fclose(stream);

  assert(close_result == 0);
}

static void test_rejects_invalid_arguments(void)
{
  Cpu cpu = {0};
  const Program program = get_test_program();

  const Program null_program =
  {
    .bytes = NULL,
    .size = 1
  };

  const Program oversized_program =
  {
    .bytes = TEST_PROGRAM_BYTES,
    .size = CPU_PROGRAM_MEMORY_SIZE + 1
  };

  FILE *const input = create_input_stream("");
  FILE *const output = test_tmpfile();
  FILE *const error_output = test_tmpfile();

  assert(output != NULL);
  assert(error_output != NULL);

  const bool null_cpu_accepted = monitor_run(
    NULL,
    program,
    input,
    output,
    error_output
  );

  const bool null_input_accepted = monitor_run(
    &cpu,
    program,
    NULL,
    output,
    error_output
  );

  const bool null_output_accepted = monitor_run(
    &cpu,
    program,
    input,
    NULL,
    error_output
  );

  const bool null_error_accepted = monitor_run(
    &cpu,
    program,
    input,
    output,
    NULL
  );

  const bool null_program_accepted = monitor_run(
    &cpu,
    null_program,
    input,
    output,
    error_output
  );

  const bool oversized_program_accepted = monitor_run(
    &cpu,
    oversized_program,
    input,
    output,
    error_output
  );

  assert(!null_cpu_accepted);
  assert(!null_input_accepted);
  assert(!null_output_accepted);
  assert(!null_error_accepted);
  assert(!null_program_accepted);
  assert(!oversized_program_accepted);

  close_stream(input);
  close_stream(output);
  close_stream(error_output);
}

static void test_executes_basic_session(void)
{
  static const char commands[] =
    "  help  \n"
    "registers\n"
    "unknown\n"
    "  quit  \n";

  static const char expected_output[] =
    "VM8 interactive monitor\n"
    "Type 'help' to list the available commands.\n"
    "vm8> Monitor commands:\n"
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
    "  quit                         Leave the monitor.\n"
    "vm8> Register A: 0x00\n"
    "Register B: 0x00\n"
    "Stack pointer: 0x00\n"
    "Input port: 0x00\n"
    "Output port: 0x00\n"
    "Zero flag: clear\n"
    "Carry flag: clear\n"
    "Program counter: 0\n"
    "Cycle count: 0\n"
    "vm8> vm8> Leaving VM8 monitor.\n";

  static const char expected_error[] =
    "Unknown monitor command: 'unknown'. Type 'help'.\n";

  Cpu cpu = {0};
  const Program program = get_test_program();

  const bool loaded = cpu_load_program(
    &cpu,
    program.bytes,
    program.size
  );

  assert(loaded);

  FILE *const input = create_input_stream(commands);
  FILE *const output = test_tmpfile();
  FILE *const error_output = test_tmpfile();

  assert(output != NULL);
  assert(error_output != NULL);

  const bool succeeded = monitor_run(
    &cpu,
    program,
    input,
    output,
    error_output
  );

  assert(succeeded);
  assert_stream_equals(output, expected_output);
  assert_stream_equals(error_output, expected_error);
  assert(cpu.program_counter == 0);
  assert(cpu.cycle_count == 0);

  close_stream(input);
  close_stream(output);
  close_stream(error_output);
}

static void test_controls_cpu_execution(void)
{
  static const char commands[] =
    "input 0xA5\n"
    "step\n"
    "run\n"
    "reset\n"
    "registers\n"
    "quit\n";

  static const char expected_output[] =
    "VM8 interactive monitor\n"
    "Type 'help' to list the available commands.\n"
    "vm8> Input port set to 0xA5.\n"
    "vm8> Step result: ok\n"
    "Register A: 0x2A\n"
    "Register B: 0x00\n"
    "Stack pointer: 0x00\n"
    "Input port: 0xA5\n"
    "Output port: 0x00\n"
    "Zero flag: clear\n"
    "Carry flag: clear\n"
    "Program counter: 2\n"
    "Cycle count: 1\n"
    "vm8> Execution result: halted\n"
    "Register A: 0x2A\n"
    "Register B: 0x00\n"
    "Stack pointer: 0x00\n"
    "Input port: 0xA5\n"
    "Output port: 0x00\n"
    "Zero flag: clear\n"
    "Carry flag: clear\n"
    "Program counter: 3\n"
    "Cycle count: 2\n"
    "vm8> CPU reset and program reloaded.\n"
    "vm8> Register A: 0x00\n"
    "Register B: 0x00\n"
    "Stack pointer: 0x00\n"
    "Input port: 0x00\n"
    "Output port: 0x00\n"
    "Zero flag: clear\n"
    "Carry flag: clear\n"
    "Program counter: 0\n"
    "Cycle count: 0\n"
    "vm8> Leaving VM8 monitor.\n";

  Cpu cpu = {0};
  const Program program = get_test_program();

  const bool loaded = cpu_load_program(
    &cpu,
    program.bytes,
    program.size
  );

  assert(loaded);

  FILE *const input = create_input_stream(commands);
  FILE *const output = test_tmpfile();
  FILE *const error_output = test_tmpfile();

  assert(output != NULL);
  assert(error_output != NULL);

  const bool succeeded = monitor_run(
    &cpu,
    program,
    input,
    output,
    error_output
  );

  assert(succeeded);
  assert_stream_equals(output, expected_output);
  assert_stream_equals(error_output, "");
  assert(cpu.register_a == 0);
  assert(cpu.program_counter == 0);
  assert(cpu.cycle_count == 0);
  assert(cpu_read_memory(&cpu, 0) == OPCODE_LOAD_IMMEDIATE_A);

  close_stream(input);
  close_stream(output);
  close_stream(error_output);
}

static void test_manages_breakpoints_and_trace(void)
{
  static const char commands[] =
    "breakpoint list\n"
    "breakpoint add 0x04\n"
    "breakpoint add 4\n"
    "trace\n"
    "trace on\n"
    "run\n"
    "step\n"
    "breakpoint list\n"
    "reset\n"
    "run\n"
    "breakpoint remove 0x04\n"
    "breakpoint remove 0x04\n"
    "breakpoint clear\n"
    "trace off\n"
    "trace\n"
    "quit\n";

  Cpu cpu = {0};
  const Program program = get_breakpoint_test_program();

  const bool loaded = cpu_load_program(
    &cpu,
    program.bytes,
    program.size
  );

  assert(loaded);

  FILE *const input = create_input_stream(commands);
  FILE *const output = test_tmpfile();
  FILE *const error_output = test_tmpfile();

  assert(output != NULL);
  assert(error_output != NULL);

  const bool succeeded = monitor_run(
    &cpu,
    program,
    input,
    output,
    error_output
  );

  assert(succeeded);
  assert_stream_contains(output, "No breakpoints set.\n");
  assert_stream_contains(output, "Breakpoint added at 0x04.\n");
  assert_stream_contains(output, "Breakpoint already set at 0x04.\n");
  assert_stream_contains(output, "Trace enabled.\n");
  assert_stream_contains(output, "Breakpoints:\n  0x04\n");
  assert_stream_contains(output, "CPU reset and program reloaded.\n");
  assert_stream_contains(output, "Breakpoint removed from 0x04.\n");
  assert_stream_contains(output, "No breakpoint set at 0x04.\n");
  assert_stream_contains(output, "All breakpoints cleared.\n");
  assert_stream_contains(output, "Trace disabled.\n");
  assert_stream_fragment_count(output, "Trace is off.\n", 2);
  assert_stream_fragment_count(output, "Execution trace:\n", 3);
  assert_stream_fragment_count(
    output,
    "ADDR=0x00 OP=0x10 MNEMONIC=LDI",
    2
  );
  assert_stream_fragment_count(
    output,
    "ADDR=0x02 OP=0x11 MNEMONIC=LDI",
    2
  );
  assert_stream_fragment_count(
    output,
    "ADDR=0x04 OP=0x20 MNEMONIC=ADD",
    1
  );
  assert_stream_fragment_count(
    output,
    "Breakpoint reached at 0x04.\n",
    2
  );
  assert_stream_equals(error_output, "");
  assert(cpu.register_a == TEST_LOADED_VALUE);
  assert(cpu.register_b == TEST_LOADED_VALUE);
  assert(cpu.program_counter == TEST_BREAKPOINT_ADDRESS);
  assert(cpu.cycle_count == 2);
  assert(!cpu.halted);

  close_stream(input);
  close_stream(output);
  close_stream(error_output);
}

static void test_reads_memory_without_address_wraparound(void)
{
  static const char commands[] =
    "memory 0 3\n"
    "memory 0xEE 4\n"
    "memory 0xFE 4\n"
    "quit\n";

  static const char expected_output[] =
    "VM8 interactive monitor\n"
    "Type 'help' to list the available commands.\n"
    "vm8> 0x00: 10 2A 01\n"
    "vm8> 0xEE: A5 5A 00 00\n"
    "vm8> 0xFE: 00 00\n"
    "vm8> Leaving VM8 monitor.\n";

  Cpu cpu = {0};
  const Program program = get_test_program();

  const bool loaded = cpu_load_program(
    &cpu,
    program.bytes,
    program.size
  );

  assert(loaded);

  cpu_set_input_port(&cpu, TEST_INPUT_VALUE);

  cpu_write_memory(
    &cpu,
    CPU_OUTPUT_PORT_ADDRESS,
    TEST_OUTPUT_VALUE
  );

  FILE *const input = create_input_stream(commands);
  FILE *const output = test_tmpfile();
  FILE *const error_output = test_tmpfile();

  assert(output != NULL);
  assert(error_output != NULL);

  const bool succeeded = monitor_run(
    &cpu,
    program,
    input,
    output,
    error_output
  );

  assert(succeeded);
  assert_stream_equals(output, expected_output);
  assert_stream_equals(error_output, "");

  close_stream(input);
  close_stream(output);
  close_stream(error_output);
}

static void test_loads_and_resets_binary_program(void)
{
  write_binary_file(
    TEST_BINARY_PATH,
    TEST_REPLACEMENT_PROGRAM_BYTES,
    sizeof TEST_REPLACEMENT_PROGRAM_BYTES
  );

  char commands[TEST_COMMAND_BUFFER_CAPACITY] = {0};

  const int command_length = snprintf(
    commands,
    sizeof commands,
    "load %s\nrun\nreset\nstep\nquit\n",
    TEST_BINARY_PATH
  );

  assert(command_length >= 0);
  assert((size_t)command_length < sizeof commands);

  Cpu cpu = {0};
  const Program program = get_test_program();

  const bool initial_program_loaded = cpu_load_program(
    &cpu,
    program.bytes,
    program.size
  );

  assert(initial_program_loaded);

  FILE *const input = create_input_stream(commands);
  FILE *const output = test_tmpfile();
  FILE *const error_output = test_tmpfile();

  assert(output != NULL);
  assert(error_output != NULL);

  const bool succeeded = monitor_run(
    &cpu,
    program,
    input,
    output,
    error_output
  );

  char expected_load_message[TEST_COMMAND_BUFFER_CAPACITY] = {0};

  const int message_length = snprintf(
    expected_load_message,
    sizeof expected_load_message,
    "Loaded %zu byte(s) from '%s'; CPU reset.\n",
    sizeof TEST_REPLACEMENT_PROGRAM_BYTES,
    TEST_BINARY_PATH
  );

  assert(message_length >= 0);
  assert((size_t)message_length < sizeof expected_load_message);
  assert(succeeded);
  assert_stream_contains(output, expected_load_message);
  assert_stream_contains(output, "Execution result: halted\n");
  assert_stream_contains(output, "Register B: 0x5A\n");
  assert_stream_contains(output, "CPU reset and program reloaded.\n");
  assert_stream_contains(output, "Step result: ok\n");
  assert_stream_equals(error_output, "");
  assert(cpu.register_a == 0);
  assert(cpu.register_b == TEST_REPLACEMENT_VALUE);
  assert(cpu.program_counter == 2);
  assert(cpu.cycle_count == 1);

  close_stream(input);
  close_stream(output);
  close_stream(error_output);

  const int remove_result = remove(TEST_BINARY_PATH);

  assert(remove_result == 0);
}

static void test_preserves_program_after_empty_load(void)
{
  write_binary_file(TEST_EMPTY_BINARY_PATH, NULL, 0);

  char commands[TEST_COMMAND_BUFFER_CAPACITY] = {0};

  const int command_length = snprintf(
    commands,
    sizeof commands,
    "load %s\nreset\nstep\nquit\n",
    TEST_EMPTY_BINARY_PATH
  );

  assert(command_length >= 0);
  assert((size_t)command_length < sizeof commands);

  Cpu cpu = {0};
  const Program program = get_test_program();

  const bool initial_program_loaded = cpu_load_program(
    &cpu,
    program.bytes,
    program.size
  );

  assert(initial_program_loaded);

  FILE *const input = create_input_stream(commands);
  FILE *const output = test_tmpfile();
  FILE *const error_output = test_tmpfile();

  assert(output != NULL);
  assert(error_output != NULL);

  const bool succeeded = monitor_run(
    &cpu,
    program,
    input,
    output,
    error_output
  );

  char expected_error[TEST_COMMAND_BUFFER_CAPACITY] = {0};

  const int message_length = snprintf(
    expected_error,
    sizeof expected_error,
    "%s: binary program is empty\n",
    TEST_EMPTY_BINARY_PATH
  );

  assert(message_length >= 0);
  assert((size_t)message_length < sizeof expected_error);
  assert(succeeded);
  assert_stream_contains(output, "CPU reset and program reloaded.\n");
  assert_stream_contains(output, "Step result: ok\n");
  assert_stream_equals(error_output, expected_error);
  assert(cpu.register_a == TEST_LOADED_VALUE);
  assert(cpu.program_counter == 2);
  assert(cpu.cycle_count == 1);

  close_stream(input);
  close_stream(output);
  close_stream(error_output);

  const int remove_result = remove(TEST_EMPTY_BINARY_PATH);

  assert(remove_result == 0);
}

static void test_rejects_invalid_command_arguments(void)
{
  static const char commands[] =
    "input\n"
    "input 256\n"
    "step extra\n"
    "memory\n"
    "memory 256\n"
    "memory 0 0\n"
    "trace maybe\n"
    "breakpoint add\n"
    "breakpoint add 256\n"
    "quit\n";

  static const char expected_output[] =
    "VM8 interactive monitor\n"
    "Type 'help' to list the available commands.\n"
    "vm8> vm8> vm8> vm8> vm8> vm8> vm8> "
    "vm8> vm8> vm8> "
    "Leaving VM8 monitor.\n";

  static const char expected_error[] =
    "Usage: input <byte>\n"
    "The byte must be decimal or 0x-prefixed hexadecimal.\n"
    "Usage: input <byte>\n"
    "The byte must be decimal or 0x-prefixed hexadecimal.\n"
    "Usage: step\n"
    "Usage: memory <address> [count]\n"
    "Usage: memory <address> [count]\n"
    "The address must be an 8-bit value.\n"
    "Usage: memory <address> [count]\n"
    "The count must be from 1 through 255.\n"
    "Usage: trace [on|off]\n"
    "Usage:\n"
    "  breakpoint add <address>\n"
    "  breakpoint remove <address>\n"
    "  breakpoint list\n"
    "  breakpoint clear\n"
    "Breakpoint address must be an 8-bit value.\n";

  Cpu cpu = {0};
  const Program program = get_test_program();
  FILE *const input = create_input_stream(commands);
  FILE *const output = test_tmpfile();
  FILE *const error_output = test_tmpfile();

  assert(output != NULL);
  assert(error_output != NULL);

  const bool succeeded = monitor_run(
    &cpu,
    program,
    input,
    output,
    error_output
  );

  assert(succeeded);
  assert_stream_equals(output, expected_output);
  assert_stream_equals(error_output, expected_error);
  assert(cpu.input_port == 0);
  assert(cpu.cycle_count == 0);

  close_stream(input);
  close_stream(output);
  close_stream(error_output);
}

static void test_accepts_end_of_file(void)
{
  static const char expected_output[] =
    "VM8 interactive monitor\n"
    "Type 'help' to list the available commands.\n"
    "vm8> \n";

  Cpu cpu = {0};
  const Program program = get_test_program();
  FILE *const input = create_input_stream("");
  FILE *const output = test_tmpfile();
  FILE *const error_output = test_tmpfile();

  assert(output != NULL);
  assert(error_output != NULL);

  const bool succeeded = monitor_run(
    &cpu,
    program,
    input,
    output,
    error_output
  );

  assert(succeeded);
  assert_stream_equals(output, expected_output);
  assert_stream_equals(error_output, "");

  close_stream(input);
  close_stream(output);
  close_stream(error_output);
}

static void test_discards_overlong_command(void)
{
  static const char expected_output[] =
    "VM8 interactive monitor\n"
    "Type 'help' to list the available commands.\n"
    "vm8> vm8> Leaving VM8 monitor.\n";

  static const char expected_error[] =
    "Monitor command exceeds 255 characters.\n";

  Cpu cpu = {0};
  const Program program = get_test_program();
  FILE *const input = test_tmpfile();
  FILE *const output = test_tmpfile();
  FILE *const error_output = test_tmpfile();

  assert(input != NULL);
  assert(output != NULL);
  assert(error_output != NULL);

  for (
    size_t index = 0;
    index < MONITOR_OVERLONG_COMMAND_LENGTH;
    ++index
  )
  {
    const int write_result = fputc('x', input);

    assert(write_result != EOF);
  }

  const int command_write_result = fputs("\nquit\n", input);

  assert(command_write_result >= 0);

  const int flush_result = fflush(input);

  assert(flush_result == 0);

  rewind(input);

  const bool succeeded = monitor_run(
    &cpu,
    program,
    input,
    output,
    error_output
  );

  assert(succeeded);
  assert_stream_equals(output, expected_output);
  assert_stream_equals(error_output, expected_error);

  close_stream(input);
  close_stream(output);
  close_stream(error_output);
}

int main(void)
{
  test_rejects_invalid_arguments();
  test_executes_basic_session();
  test_controls_cpu_execution();
  test_manages_breakpoints_and_trace();
  test_reads_memory_without_address_wraparound();
  test_loads_and_resets_binary_program();
  test_preserves_program_after_empty_load();
  test_rejects_invalid_command_arguments();
  test_accepts_end_of_file();
  test_discards_overlong_command();

  puts("All monitor tests passed.");

  return EXIT_SUCCESS;
}
