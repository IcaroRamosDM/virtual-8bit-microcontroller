CC := gcc
.DEFAULT_GOAL := all

CPPFLAGS := -Iinclude -Iassembler
C_STANDARD := -std=c17
WARNINGS := -Wall -Wextra -Wpedantic -Werror
DEBUG_FLAGS := -g
CFLAGS := $(C_STANDARD) $(WARNINGS) $(DEBUG_FLAGS)

WINDOWS_CC ?= x86_64-w64-mingw32-gcc
WINDOWS_CXX ?= x86_64-w64-mingw32-g++
WINDOWS_CPPFLAGS ?= -D__USE_MINGW_ANSI_STDIO=1
# Use the MinGW headers' GNU format checks, not GCC's legacy MSVCRT built-ins.
WINDOWS_CFLAGS ?= -fno-builtin-printf -fno-builtin-fprintf -fno-builtin-snprintf
RELEASE_CFLAGS ?= -O2
RELEASE_LDFLAGS ?= -s
WINDOWS_LDFLAGS ?= -static
LINUX_BUILD_DIR := build/linux
WINDOWS_BUILD_DIR := build/windows
WINDOWS_TEST_BUILD_DIR := build/windows-tests
LINUX_TARGET := $(LINUX_BUILD_DIR)/vm8
LINUX_ASSEMBLER_TARGET := $(LINUX_BUILD_DIR)/vm8asm
WINDOWS_TARGET := $(WINDOWS_BUILD_DIR)/vm8.exe
WINDOWS_ASSEMBLER_TARGET := $(WINDOWS_BUILD_DIR)/vm8asm.exe
BINARY_USAGE_GUIDE := docs/RUNNING_BINARIES.md

TARGET := build/vm8
ASSEMBLER_TARGET := build/vm8asm
ASSEMBLER_DEMO_SOURCE := programs/demo.asm
ASSEMBLER_DEMO_OUTPUT := build/demo.bin
POPCOUNT_SOURCE := programs/popcount.asm
POPCOUNT_OUTPUT := build/popcount.bin
PROCESS_TEST_SCRIPT := tests/test_vm8_process.sh
INPUT_VALUE ?= 0x00
CPU_TEST_TARGET := build/test_cpu
CPU_OBSERVER_TEST_TARGET := build/test_cpu_observer
INSTRUCTION_SET_TEST_TARGET := build/test_instruction_set
CPU_TRACE_TEST_TARGET := build/test_cpu_trace
CPU_STATE_TEST_TARGET := build/test_cpu_state
MONITOR_TEST_TARGET := build/test_monitor
BYTE_VALUE_TEST_TARGET := build/test_byte_value
CLI_TEST_TARGET := build/test_cli
PROGRAM_TEST_TARGET := build/test_program
BINARY_READER_TEST_TARGET := build/test_binary_reader
ASSEMBLED_PROGRAM_TEST_TARGET := build/test_assembled_program
POPCOUNT_PROGRAM_TEST_TARGET := build/test_popcount_program
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
ASSEMBLY_TEST_TARGET := build/test_assembly
TEST_TARGETS := \
	$(ASSEMBLY_TEST_TARGET) \
	$(CPU_TEST_TARGET) \
	$(CPU_OBSERVER_TEST_TARGET) \
	$(INSTRUCTION_SET_TEST_TARGET) \
	$(CPU_TRACE_TEST_TARGET) \
	$(CPU_STATE_TEST_TARGET) \
	$(MONITOR_TEST_TARGET) \
	$(BYTE_VALUE_TEST_TARGET) \
	$(CLI_TEST_TARGET) \
	$(PROGRAM_TEST_TARGET) \
	$(BINARY_READER_TEST_TARGET) \
	$(ASSEMBLED_PROGRAM_TEST_TARGET) \
	$(POPCOUNT_PROGRAM_TEST_TARGET) \
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

WINDOWS_TEST_TARGETS := $(patsubst build/%,$(WINDOWS_TEST_BUILD_DIR)/%.exe,$(TEST_TARGETS))
WINDOWS_TEST_OVERRIDES := $(foreach name,$(filter %_TEST_TARGET,$(.VARIABLES)),$(name)=$(patsubst build/%,$(WINDOWS_TEST_BUILD_DIR)/%.exe,$($(name))))

SOURCES := $(wildcard src/*.c)
HEADERS := $(wildcard include/*.h)
ASSEMBLER_SOURCES := $(wildcard assembler/*.c)
ASSEMBLER_HEADERS := $(wildcard assembler/*.h)
CPU_TEST_SOURCES := src/cpu.c tests/test_cpu.c
CPU_OBSERVER_TEST_SOURCES := src/cpu.c tests/test_cpu_observer.c
INSTRUCTION_SET_TEST_SOURCES := src/instruction_set.c tests/test_instruction_set.c
CPU_TRACE_TEST_SOURCES := src/cpu.c src/cpu_trace.c src/instruction_set.c tests/test_cpu_trace.c
CPU_STATE_TEST_SOURCES := src/cpu.c src/cpu_state.c tests/test_cpu_state.c
MONITOR_TEST_SOURCES := \
	src/binary_reader.c \
	src/byte_value.c \
	src/cpu.c \
	src/cpu_state.c \
	src/cpu_trace.c \
	src/instruction_set.c \
	src/monitor.c \
	tests/test_monitor.c
BYTE_VALUE_TEST_SOURCES := src/byte_value.c tests/test_byte_value.c
CLI_TEST_SOURCES := src/byte_value.c src/cli.c tests/test_cli.c
PROGRAM_TEST_SOURCES := src/cpu.c src/program.c tests/test_program.c
BINARY_READER_TEST_SOURCES := src/binary_reader.c tests/test_binary_reader.c
ASSEMBLED_PROGRAM_TEST_SOURCES := src/binary_reader.c src/cpu.c tests/test_assembled_program.c
POPCOUNT_PROGRAM_TEST_SOURCES := src/binary_reader.c src/cpu.c tests/test_popcount_program.c
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
ASSEMBLY_TEST_SOURCES := $(filter-out assembler/main.c,$(ASSEMBLER_SOURCES)) tests/test_assembly.c

.PHONY: studio studio-linux studio-windows studio-deps-linux studio-deps-windows
studio: studio-linux studio-windows
studio-linux: $(TARGET)
	python3 tools/build_studio.py build linux
studio-windows: $(TARGET)
	python3 tools/build_studio.py build windows
studio-deps-linux:
	python3 tools/build_studio.py deps linux
studio-deps-windows:
	python3 tools/build_studio.py deps windows

.PHONY: studio-appimage studio-test studio-timing-test studio-timing-windows-build
studio-appimage:
	python3 tools/package_studio.py all

# Reuse the finished AppImage; desktop setup does not rebuild the C core or GUI.
.PHONY: studio-deb studio-install-user studio-desktop-test
studio-deb:
	python3 tools/linux_desktop.py deb

studio-install-user:
	python3 tools/linux_desktop.py install-user --desktop-shortcut

studio-desktop-test:
	python3 -B -m unittest discover -s tests -p 'test_linux_desktop.py' -v

STUDIO_TIMING_SOURCES := studio/timing.cpp tests/test_studio_timing.cpp
STUDIO_TIMING_FLAGS := -std=c++17 $(WARNINGS) -O2

build/test_studio_timing: $(STUDIO_TIMING_SOURCES) studio/timing.h Makefile
	mkdir -p build
	$(CXX) -Istudio $(STUDIO_TIMING_FLAGS) $(STUDIO_TIMING_SOURCES) -o $@

build/windows-tests/test_studio_timing.exe: $(STUDIO_TIMING_SOURCES) studio/timing.h Makefile
	mkdir -p build/windows-tests
	$(WINDOWS_CXX) -Istudio $(STUDIO_TIMING_FLAGS) -static $(STUDIO_TIMING_SOURCES) -o $@

studio-timing-test: build/test_studio_timing
	./build/test_studio_timing

studio-timing-windows-build: build/windows-tests/test_studio_timing.exe

studio-test: studio-linux studio-timing-test
	mkdir -p build/studio-smoke
	./build/studio-linux/vm8-studio --self-test
	xvfb-run -a -s '-screen 0 1600x1000x24' ./build/studio-linux/vm8-studio --ui-smoke-test build/studio-smoke

.PHONY: all assembler assemble assemble-popcount inspect inspect-popcount run run-bin run-popcount trace trace-bin trace-popcount monitor monitor-bin monitor-popcount test help linux windows windows-test-build release clean

all: $(TARGET)

assembler: $(ASSEMBLER_TARGET)

release: linux windows

windows-test-build: assemble assemble-popcount
	mkdir -p $(WINDOWS_TEST_BUILD_DIR)
	$(MAKE) CC="$(WINDOWS_CC)" \
		CPPFLAGS="$(CPPFLAGS) $(WINDOWS_CPPFLAGS)" \
		CFLAGS="$(C_STANDARD) $(WARNINGS) $(WINDOWS_CFLAGS) $(RELEASE_CFLAGS) $(WINDOWS_LDFLAGS)" \
		$(WINDOWS_TEST_OVERRIDES) $(WINDOWS_TEST_TARGETS)

linux: $(LINUX_TARGET) $(LINUX_ASSEMBLER_TARGET) assemble assemble-popcount $(BINARY_USAGE_GUIDE) LICENSE
	cp $(ASSEMBLER_DEMO_OUTPUT) $(POPCOUNT_OUTPUT) LICENSE $(LINUX_BUILD_DIR)/
	cp $(ASSEMBLER_DEMO_SOURCE) $(POPCOUNT_SOURCE) $(LINUX_BUILD_DIR)/
	cp $(BINARY_USAGE_GUIDE) $(LINUX_BUILD_DIR)/USAGE.md

windows: $(WINDOWS_TARGET) $(WINDOWS_ASSEMBLER_TARGET) assemble assemble-popcount $(BINARY_USAGE_GUIDE) LICENSE
	cp $(ASSEMBLER_DEMO_OUTPUT) $(POPCOUNT_OUTPUT) LICENSE $(WINDOWS_BUILD_DIR)/
	cp $(ASSEMBLER_DEMO_SOURCE) $(POPCOUNT_SOURCE) $(WINDOWS_BUILD_DIR)/
	cp $(BINARY_USAGE_GUIDE) $(WINDOWS_BUILD_DIR)/USAGE.md

$(LINUX_BUILD_DIR) $(WINDOWS_BUILD_DIR):
	mkdir -p $@

$(LINUX_TARGET): $(SOURCES) $(HEADERS) Makefile | $(LINUX_BUILD_DIR)
	$(CC) $(CPPFLAGS) $(C_STANDARD) $(WARNINGS) $(RELEASE_CFLAGS) $(SOURCES) $(RELEASE_LDFLAGS) -o $@

$(LINUX_ASSEMBLER_TARGET): $(ASSEMBLER_SOURCES) $(ASSEMBLER_HEADERS) $(HEADERS) Makefile | $(LINUX_BUILD_DIR)
	$(CC) $(CPPFLAGS) $(C_STANDARD) $(WARNINGS) $(RELEASE_CFLAGS) $(ASSEMBLER_SOURCES) $(RELEASE_LDFLAGS) -o $@

$(WINDOWS_TARGET): $(SOURCES) $(HEADERS) Makefile | $(WINDOWS_BUILD_DIR)
	$(WINDOWS_CC) $(CPPFLAGS) $(WINDOWS_CPPFLAGS) $(C_STANDARD) $(WARNINGS) $(WINDOWS_CFLAGS) $(RELEASE_CFLAGS) $(SOURCES) $(WINDOWS_LDFLAGS) $(RELEASE_LDFLAGS) -o $@

$(WINDOWS_ASSEMBLER_TARGET): $(ASSEMBLER_SOURCES) $(ASSEMBLER_HEADERS) $(HEADERS) Makefile | $(WINDOWS_BUILD_DIR)
	$(WINDOWS_CC) $(CPPFLAGS) $(WINDOWS_CPPFLAGS) $(C_STANDARD) $(WARNINGS) $(WINDOWS_CFLAGS) $(RELEASE_CFLAGS) $(ASSEMBLER_SOURCES) $(WINDOWS_LDFLAGS) $(RELEASE_LDFLAGS) -o $@

assemble: $(ASSEMBLER_TARGET) $(ASSEMBLER_DEMO_SOURCE)
	./$(ASSEMBLER_TARGET) $(ASSEMBLER_DEMO_SOURCE) $(ASSEMBLER_DEMO_OUTPUT)

assemble-popcount: $(ASSEMBLER_TARGET) $(POPCOUNT_SOURCE)
	./$(ASSEMBLER_TARGET) $(POPCOUNT_SOURCE) $(POPCOUNT_OUTPUT)

inspect: assemble
	wc -c $(ASSEMBLER_DEMO_OUTPUT)
	od -An -tx1 -v $(ASSEMBLER_DEMO_OUTPUT)

inspect-popcount: assemble-popcount
	wc -c $(POPCOUNT_OUTPUT)
	od -An -tx1 -v $(POPCOUNT_OUTPUT)

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

$(CPU_TRACE_TEST_TARGET): $(CPU_TRACE_TEST_SOURCES) $(HEADERS) tests/test_stream.h
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CPU_TRACE_TEST_SOURCES) -o $(CPU_TRACE_TEST_TARGET)

$(CPU_STATE_TEST_TARGET): $(CPU_STATE_TEST_SOURCES) $(HEADERS) tests/test_stream.h
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CPU_STATE_TEST_SOURCES) -o $(CPU_STATE_TEST_TARGET)

$(MONITOR_TEST_TARGET): $(MONITOR_TEST_SOURCES) $(HEADERS) tests/test_stream.h
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(MONITOR_TEST_SOURCES) -o $(MONITOR_TEST_TARGET)

$(BYTE_VALUE_TEST_TARGET): $(BYTE_VALUE_TEST_SOURCES) $(HEADERS)
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(BYTE_VALUE_TEST_SOURCES) -o $(BYTE_VALUE_TEST_TARGET)

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

$(POPCOUNT_PROGRAM_TEST_TARGET): $(POPCOUNT_PROGRAM_TEST_SOURCES) $(HEADERS)
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(POPCOUNT_PROGRAM_TEST_SOURCES) -o $(POPCOUNT_PROGRAM_TEST_TARGET)

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

$(ASSEMBLY_TEST_TARGET): $(ASSEMBLY_TEST_SOURCES) $(ASSEMBLER_HEADERS) $(HEADERS)
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(ASSEMBLY_TEST_SOURCES) -o $@

$(TEST_TARGETS): Makefile

run: $(TARGET)
	./$(TARGET) run --input $(INPUT_VALUE)

run-bin: $(TARGET) assemble
	./$(TARGET) run $(ASSEMBLER_DEMO_OUTPUT) --input $(INPUT_VALUE)

run-popcount: $(TARGET) assemble-popcount
	./$(TARGET) run $(POPCOUNT_OUTPUT) --input $(INPUT_VALUE)

trace: $(TARGET)
	./$(TARGET) trace --input $(INPUT_VALUE)

trace-bin: $(TARGET) assemble
	./$(TARGET) trace $(ASSEMBLER_DEMO_OUTPUT) --input $(INPUT_VALUE)

trace-popcount: $(TARGET) assemble-popcount
	./$(TARGET) trace $(POPCOUNT_OUTPUT) --input $(INPUT_VALUE)

monitor: $(TARGET)
	./$(TARGET) monitor

monitor-bin: $(TARGET) assemble
	./$(TARGET) monitor $(ASSEMBLER_DEMO_OUTPUT)

monitor-popcount: $(TARGET) assemble-popcount
	./$(TARGET) monitor $(POPCOUNT_OUTPUT)

test: $(TARGET) assemble assemble-popcount $(TEST_TARGETS) $(PROCESS_TEST_SCRIPT)
	./$(ASSEMBLY_TEST_TARGET)
	./$(CPU_TEST_TARGET)
	./$(CPU_OBSERVER_TEST_TARGET)
	./$(INSTRUCTION_SET_TEST_TARGET)
	./$(CPU_TRACE_TEST_TARGET)
	./$(CPU_STATE_TEST_TARGET)
	./$(MONITOR_TEST_TARGET)
	./$(BYTE_VALUE_TEST_TARGET)
	./$(CLI_TEST_TARGET)
	./$(PROGRAM_TEST_TARGET)
	./$(BINARY_READER_TEST_TARGET)
	./$(ASSEMBLED_PROGRAM_TEST_TARGET)
	./$(POPCOUNT_PROGRAM_TEST_TARGET)
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
	@printf '\nVM8 Studio graphical application (no tools needed by end users):\n'
	@printf '  make studio-windows   Build the portable Windows x64 GUI executable\n'
	@printf '  make studio-linux     Build a GUI executable for the current Linux host\n'
	@printf '  make studio-appimage  Build the portable Linux AppImage on Debian 12\n'
	@printf '  make studio-deb       Package the existing AppImage as an Ubuntu/Debian installer\n'
	@printf '  make studio-install-user  Install Studio in your account, with menu and desktop icons\n'
	@printf '  make studio-desktop-test  Test Linux installer and launcher behavior\n'
	@printf '  make studio-test      Test GUI actions on a virtual display\n'
	@printf '  make studio-timing-test  Test virtual clock and real stopwatch without a display\n'
	@printf '  make studio-timing-windows-build  Build the native Windows stopwatch tests\n'
	@printf '  Studio: F1 opens the complete offline reference; Ctrl+Enter assembles; F5 runs\n'
	@printf '  Studio: Clock (Hz) calculates virtual time; Real elapsed excludes pauses; Speed controls pacing\n'
	@printf '  Guide: docs/STUDIO.md\n'
	@printf '\nNative executable builds (run Make inside Linux/WSL):\n'
	@printf '  make linux    Build Linux executables in build/linux\n'
	@printf '  make windows  Cross-compile Windows x64 executables in build/windows\n'
	@printf '  make release  Build both platforms, examples, license, and usage guide\n'
	@printf '  make windows-test-build  Compile C tests for execution on Windows\n'
	@printf '  Guide: docs/RUNNING_BINARIES.md\n'

clean:
	$(RM) -r build
