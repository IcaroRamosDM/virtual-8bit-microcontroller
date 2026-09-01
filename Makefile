CC := gcc

CPPFLAGS := -Iinclude
C_STANDARD := -std=c17
WARNINGS := -Wall -Wextra -Wpedantic -Werror
DEBUG_FLAGS := -g
CFLAGS := $(C_STANDARD) $(WARNINGS) $(DEBUG_FLAGS)

TARGET := build/vm8
SOURCES := $(wildcard src/*.c)
HEADERS := $(wildcard include/*.h)

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	$(RM) -r build
