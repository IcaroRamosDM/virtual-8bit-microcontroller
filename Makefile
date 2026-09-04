CC := gcc

CPPFLAGS := -Iinclude -Iassembler
C_STANDARD := -std=c17
WARNINGS := -Wall -Wextra -Wpedantic -Werror
DEBUG_FLAGS := -g
CFLAGS := $(C_STANDARD) $(WARNINGS) $(DEBUG_FLAGS)

TARGET := build/vm8
ASSEMBLER_TARGET := build/vm8asm
ASSEMBLER_DEMO_SOURCE := programs/demo.asm
ASSEMBLER_DEMO_OUTPUT := build/demo.bin
PROCESS_TEST_SCRIPT := tests/test_vm8_process.sh
CPU_TEST_TARGET := build/test_cpu
CPU_OBSERVER_TEST_TARGET := build/test_cpu_observer
INSTRUCTION_SET_TEST_TARGET := build/test_instruction_set
CPU_TRACE_TEST_TARGET := build/test_cpu_trace
CLI_TEST_TARGET := build/test_cli
PROGRAM_TEST_TARGET := build/test_program
BINARY_READER_TEST_TARGET := build/test_binary_reader
ASSEMBLED_PROGRAM_TEST_TARGET := build/test_assembled_program
SOURCE_LINE_TEST_TARGET := build/test_source_line
SOURCE_READER_TEST_TARGET := build/test_source_reader
SYMBOL_TABLE_TEST_TARGET := build/test_symbol_table
FIRST_PASS_TEST_TARGET := build/test_first_pass
BYTE_LITERAL_TEST_TARGET := build/test_byte_literal
BYTE_OPERAND_TEST_TARGET := build/test_byte_operand
INSTRUCTION_PARSER_TEST_TARGET := build/test_instruction_parser
INSTRUCTION_ENCODER_TEST_TARGET := build/test_instruction_encoder
SECOND_PASS_TEST_TARGET := build/test_second_pass
BINARY_WRITER_TEST_TARGET := build/test_binary_writer
TEST_TARGETS := \
	$(CPU_TEST_TARGET) \
	$(CPU_OBSERVER_TEST_TARGET) \
	$(INSTRUCTION_SET_TEST_TARGET) \
	$(CPU_TRACE_TEST_TARGET) \
	$(CLI_TEST_TARGET) \
	$(PROGRAM_TEST_TARGET) \
	$(BINARY_READER_TEST_TARGET) \
	$(ASSEMBLED_PROGRAM_TEST_TARGET) \
	$(SOURCE_LINE_TEST_TARGET) \
	$(SOURCE_READER_TEST_TARGET) \
	$(SYMBOL_TABLE_TEST_TARGET) \
	$(FIRST_PASS_TEST_TARGET) \
	$(BYTE_LITERAL_TEST_TARGET) \
	$(BYTE_OPERAND_TEST_TARGET) \
	$(INSTRUCTION_PARSER_TEST_TARGET) \
	$(INSTRUCTION_ENCODER_TEST_TARGET) \
	$(SECOND_PASS_TEST_TARGET) \
	$(BINARY_WRITER_TEST_TARGET)

SOURCES := $(wildcard src/*.c)
HEADERS := $(wildcard include/*.h)
ASSEMBLER_SOURCES := $(wildcard assembler/*.c)
ASSEMBLER_HEADERS := $(wildcard assembler/*.h)
CPU_TEST_SOURCES := src/cpu.c tests/test_cpu.c
CPU_OBSERVER_TEST_SOURCES := src/cpu.c tests/test_cpu_observer.c
INSTRUCTION_SET_TEST_SOURCES := src/instruction_set.c tests/test_instruction_set.c
CPU_TRACE_TEST_SOURCES := src/cpu_trace.c src/instruction_set.c tests/test_cpu_trace.c
CLI_TEST_SOURCES := src/cli.c tests/test_cli.c
PROGRAM_TEST_SOURCES := src/cpu.c src/program.c tests/test_program.c
BINARY_READER_TEST_SOURCES := src/binary_reader.c tests/test_binary_reader.c
ASSEMBLED_PROGRAM_TEST_SOURCES := src/binary_reader.c src/cpu.c tests/test_assembled_program.c
SOURCE_LINE_TEST_SOURCES := assembler/source_line.c tests/test_source_line.c
SOURCE_READER_TEST_SOURCES := assembler/source_line.c assembler/source_reader.c tests/test_source_reader.c
SYMBOL_TABLE_TEST_SOURCES := assembler/symbol_table.c tests/test_symbol_table.c
FIRST_PASS_TEST_SOURCES := assembler/byte_literal.c assembler/first_pass.c assembler/instruction_parser.c assembler/symbol_table.c tests/test_first_pass.c
BYTE_LITERAL_TEST_SOURCES := assembler/byte_literal.c tests/test_byte_literal.c
BYTE_OPERAND_TEST_SOURCES := assembler/byte_literal.c assembler/byte_operand.c assembler/symbol_table.c tests/test_byte_operand.c
INSTRUCTION_PARSER_TEST_SOURCES := assembler/instruction_parser.c tests/test_instruction_parser.c
INSTRUCTION_ENCODER_TEST_SOURCES := assembler/byte_literal.c assembler/byte_operand.c assembler/instruction_encoder.c assembler/instruction_parser.c assembler/symbol_table.c tests/test_instruction_encoder.c
SECOND_PASS_TEST_SOURCES := assembler/byte_literal.c assembler/byte_operand.c assembler/instruction_encoder.c assembler/instruction_parser.c assembler/second_pass.c assembler/symbol_table.c tests/test_second_pass.c
BINARY_WRITER_TEST_SOURCES := assembler/binary_writer.c tests/test_binary_writer.c

.PHONY: all assembler assemble inspect run run-bin trace trace-bin test help clean

all: $(TARGET)

assembler: $(ASSEMBLER_TARGET)

assemble: $(ASSEMBLER_TARGET) $(ASSEMBLER_DEMO_SOURCE)
	./$(ASSEMBLER_TARGET) $(ASSEMBLER_DEMO_SOURCE) $(ASSEMBLER_DEMO_OUTPUT)

inspect: assemble
	wc -c $(ASSEMBLER_DEMO_OUTPUT)
	od -An -tx1 -v $(ASSEMBLER_DEMO_OUTPUT)

$(TARGET): $(SOURCES) $(HEADERS)
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) -o $(TARGET)

$(ASSEMBLER_TARGET): $(ASSEMBLER_SOURCES) $(ASSEMBLER_HEADERS) $(HEADERS)
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(ASSEMBLER_SOURCES) -o $(ASSEMBLER_TARGET)

$(CPU_TEST_TARGET): $(CPU_TEST_SOURCES) $(HEADERS)
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CPU_TEST_SOURCES) -o $(CPU_TEST_TARGET)

$(CPU_OBSERVER_TEST_TARGET): $(CPU_OBSERVER_TEST_SOURCES) $(HEADERS)
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CPU_OBSERVER_TEST_SOURCES) -o $(CPU_OBSERVER_TEST_TARGET)

$(INSTRUCTION_SET_TEST_TARGET): $(INSTRUCTION_SET_TEST_SOURCES) $(HEADERS)
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(INSTRUCTION_SET_TEST_SOURCES) -o $(INSTRUCTION_SET_TEST_TARGET)

$(CPU_TRACE_TEST_TARGET): $(CPU_TRACE_TEST_SOURCES) $(HEADERS)
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CPU_TRACE_TEST_SOURCES) -o $(CPU_TRACE_TEST_TARGET)

$(CLI_TEST_TARGET): $(CLI_TEST_SOURCES) $(HEADERS)
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CLI_TEST_SOURCES) -o $(CLI_TEST_TARGET)

$(PROGRAM_TEST_TARGET): $(PROGRAM_TEST_SOURCES) $(HEADERS)
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(PROGRAM_TEST_SOURCES) -o $(PROGRAM_TEST_TARGET)

$(BINARY_READER_TEST_TARGET): $(BINARY_READER_TEST_SOURCES) $(HEADERS)
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(BINARY_READER_TEST_SOURCES) -o $(BINARY_READER_TEST_TARGET)

$(ASSEMBLED_PROGRAM_TEST_TARGET): $(ASSEMBLED_PROGRAM_TEST_SOURCES) $(HEADERS)
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(ASSEMBLED_PROGRAM_TEST_SOURCES) -o $(ASSEMBLED_PROGRAM_TEST_TARGET)

$(SOURCE_LINE_TEST_TARGET): $(SOURCE_LINE_TEST_SOURCES) $(ASSEMBLER_HEADERS)
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCE_LINE_TEST_SOURCES) -o $(SOURCE_LINE_TEST_TARGET)

$(SOURCE_READER_TEST_TARGET): $(SOURCE_READER_TEST_SOURCES) $(ASSEMBLER_HEADERS)
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCE_READER_TEST_SOURCES) -o $(SOURCE_READER_TEST_TARGET)

$(SYMBOL_TABLE_TEST_TARGET): $(SYMBOL_TABLE_TEST_SOURCES) $(ASSEMBLER_HEADERS)
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SYMBOL_TABLE_TEST_SOURCES) -o $(SYMBOL_TABLE_TEST_TARGET)

$(FIRST_PASS_TEST_TARGET): $(FIRST_PASS_TEST_SOURCES) $(ASSEMBLER_HEADERS) $(HEADERS)
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(FIRST_PASS_TEST_SOURCES) -o $(FIRST_PASS_TEST_TARGET)

$(BYTE_LITERAL_TEST_TARGET): $(BYTE_LITERAL_TEST_SOURCES) $(ASSEMBLER_HEADERS)
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(BYTE_LITERAL_TEST_SOURCES) -o $(BYTE_LITERAL_TEST_TARGET)

$(BYTE_OPERAND_TEST_TARGET): $(BYTE_OPERAND_TEST_SOURCES) $(ASSEMBLER_HEADERS)
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(BYTE_OPERAND_TEST_SOURCES) -o $(BYTE_OPERAND_TEST_TARGET)

$(INSTRUCTION_PARSER_TEST_TARGET): $(INSTRUCTION_PARSER_TEST_SOURCES) $(ASSEMBLER_HEADERS)
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(INSTRUCTION_PARSER_TEST_SOURCES) -o $(INSTRUCTION_PARSER_TEST_TARGET)

$(INSTRUCTION_ENCODER_TEST_TARGET): $(INSTRUCTION_ENCODER_TEST_SOURCES) $(ASSEMBLER_HEADERS) $(HEADERS)
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(INSTRUCTION_ENCODER_TEST_SOURCES) -o $(INSTRUCTION_ENCODER_TEST_TARGET)

$(SECOND_PASS_TEST_TARGET): $(SECOND_PASS_TEST_SOURCES) $(ASSEMBLER_HEADERS) $(HEADERS)
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SECOND_PASS_TEST_SOURCES) -o $(SECOND_PASS_TEST_TARGET)

$(BINARY_WRITER_TEST_TARGET): $(BINARY_WRITER_TEST_SOURCES) $(ASSEMBLER_HEADERS)
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(BINARY_WRITER_TEST_SOURCES) -o $(BINARY_WRITER_TEST_TARGET)

run: $(TARGET)
	./$(TARGET)

run-bin: $(TARGET) assemble
	./$(TARGET) run $(ASSEMBLER_DEMO_OUTPUT)

trace: $(TARGET)
	./$(TARGET) trace

trace-bin: $(TARGET) assemble
	./$(TARGET) trace $(ASSEMBLER_DEMO_OUTPUT)

test: $(TARGET) assemble $(TEST_TARGETS) $(PROCESS_TEST_SCRIPT)
	./$(CPU_TEST_TARGET)
	./$(CPU_OBSERVER_TEST_TARGET)
	./$(INSTRUCTION_SET_TEST_TARGET)
	./$(CPU_TRACE_TEST_TARGET)
	./$(CLI_TEST_TARGET)
	./$(PROGRAM_TEST_TARGET)
	./$(BINARY_READER_TEST_TARGET)
	./$(ASSEMBLED_PROGRAM_TEST_TARGET)
	./$(SOURCE_LINE_TEST_TARGET)
	./$(SOURCE_READER_TEST_TARGET)
	./$(SYMBOL_TABLE_TEST_TARGET)
	./$(FIRST_PASS_TEST_TARGET)
	./$(BYTE_LITERAL_TEST_TARGET)
	./$(BYTE_OPERAND_TEST_TARGET)
	./$(INSTRUCTION_PARSER_TEST_TARGET)
	./$(INSTRUCTION_ENCODER_TEST_TARGET)
	./$(SECOND_PASS_TEST_TARGET)
	./$(BINARY_WRITER_TEST_TARGET)
	bash $(PROCESS_TEST_SCRIPT)

help: $(TARGET)
	./$(TARGET) help

clean:
	$(RM) -r build
