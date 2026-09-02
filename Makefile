CC := gcc

CPPFLAGS := -Iinclude
C_STANDARD := -std=c17
WARNINGS := -Wall -Wextra -Wpedantic -Werror
DEBUG_FLAGS := -g
CFLAGS := $(C_STANDARD) $(WARNINGS) $(DEBUG_FLAGS)

TARGET := build/vm8
ASSEMBLER_TARGET := build/vm8asm
CPU_TEST_TARGET := build/test_cpu
CLI_TEST_TARGET := build/test_cli
PROGRAM_TEST_TARGET := build/test_program
TEST_TARGETS := $(CPU_TEST_TARGET) $(CLI_TEST_TARGET) $(PROGRAM_TEST_TARGET)

SOURCES := $(wildcard src/*.c)
HEADERS := $(wildcard include/*.h)
ASSEMBLER_SOURCES := $(wildcard assembler/*.c)
ASSEMBLER_HEADERS := $(wildcard assembler/*.h)
CPU_TEST_SOURCES := src/cpu.c tests/test_cpu.c
CLI_TEST_SOURCES := src/cli.c tests/test_cli.c
PROGRAM_TEST_SOURCES := src/cpu.c src/program.c tests/test_program.c

.PHONY: all assembler run test help clean

all: $(TARGET)

assembler: $(ASSEMBLER_TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) -o $(TARGET)

$(ASSEMBLER_TARGET): $(ASSEMBLER_SOURCES) $(ASSEMBLER_HEADERS) $(HEADERS)
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(ASSEMBLER_SOURCES) -o $(ASSEMBLER_TARGET)

$(CPU_TEST_TARGET): $(CPU_TEST_SOURCES) $(HEADERS)
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CPU_TEST_SOURCES) -o $(CPU_TEST_TARGET)

$(CLI_TEST_TARGET): $(CLI_TEST_SOURCES) $(HEADERS)
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CLI_TEST_SOURCES) -o $(CLI_TEST_TARGET)

$(PROGRAM_TEST_TARGET): $(PROGRAM_TEST_SOURCES) $(HEADERS)
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(PROGRAM_TEST_SOURCES) -o $(PROGRAM_TEST_TARGET)

run: $(TARGET)
	./$(TARGET)

test: $(TEST_TARGETS)
	./$(CPU_TEST_TARGET)
	./$(CLI_TEST_TARGET)
	./$(PROGRAM_TEST_TARGET)

help: $(TARGET)
	./$(TARGET) help

clean:
	$(RM) -r build
