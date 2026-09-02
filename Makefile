CC := gcc

CPPFLAGS := -Iinclude
C_STANDARD := -std=c17
WARNINGS := -Wall -Wextra -Wpedantic -Werror
DEBUG_FLAGS := -g
CFLAGS := $(C_STANDARD) $(WARNINGS) $(DEBUG_FLAGS)

TARGET := build/vm8
TEST_TARGET := build/test_cpu

SOURCES := $(wildcard src/*.c)
CORE_SOURCES := $(filter-out src/main.c,$(SOURCES))
TEST_SOURCES := tests/test_cpu.c
HEADERS := $(wildcard include/*.h)

.PHONY: all run test clean

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) -o $(TARGET)

$(TEST_TARGET): $(CORE_SOURCES) $(TEST_SOURCES) $(HEADERS)
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CORE_SOURCES) $(TEST_SOURCES) -o $(TEST_TARGET)

run: $(TARGET)
	./$(TARGET)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	$(RM) -r build
