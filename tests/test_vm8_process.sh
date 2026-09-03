#!/usr/bin/env bash

set -euo pipefail

readonly VM8_EXECUTABLE="./build/vm8"
readonly VALID_BINARY_PATH="build/demo.bin"
readonly MISSING_BINARY_PATH="build/test_vm8_missing.bin"
readonly EMPTY_BINARY_PATH="build/test_vm8_empty.bin"
readonly OVERSIZED_BINARY_PATH="build/test_vm8_oversized.bin"
readonly INVALID_OPCODE_BINARY_PATH="build/test_vm8_invalid_opcode.bin"
readonly STDOUT_PATH="build/test_vm8_stdout.txt"
readonly STDERR_PATH="build/test_vm8_stderr.txt"

readonly VM8_MEMORY_SIZE=256
readonly OVERSIZED_BINARY_SIZE=$((VM8_MEMORY_SIZE + 1))

cleanup()
{
  rm -f -- \
    "$MISSING_BINARY_PATH" \
    "$EMPTY_BINARY_PATH" \
    "$OVERSIZED_BINARY_PATH" \
    "$INVALID_OPCODE_BINARY_PATH" \
    "$STDOUT_PATH" \
    "$STDERR_PATH"
}

fail()
{
  printf 'VM8 process test failed: %s\n' "$1" >&2
  exit 1
}

expect_success()
{
  local test_name="$1"
  local expected_output="$2"

  shift 2

  if ! "$@" >"$STDOUT_PATH" 2>"$STDERR_PATH"
  then
    printf 'Standard output:\n' >&2
    cat "$STDOUT_PATH" >&2
    printf 'Standard error:\n' >&2
    cat "$STDERR_PATH" >&2
    fail "$test_name returned failure"
  fi

  if ! grep -Fq -- "$expected_output" "$STDOUT_PATH"
  then
    printf 'Standard output:\n' >&2
    cat "$STDOUT_PATH" >&2
    fail "$test_name did not produce the expected output"
  fi

  printf 'Passed: %s\n' "$test_name"
}

expect_failure()
{
  local test_name="$1"
  local expected_error="$2"

  shift 2

  if "$@" >"$STDOUT_PATH" 2>"$STDERR_PATH"
  then
    fail "$test_name returned success"
  fi

  if ! grep -Fq -- "$expected_error" "$STDERR_PATH"
  then
    printf 'Standard error:\n' >&2
    cat "$STDERR_PATH" >&2
    fail "$test_name did not produce the expected error"
  fi

  printf 'Passed: %s\n' "$test_name"
}

cleanup
trap cleanup EXIT

expect_success \
  "valid binary" \
  "Register A: 0x5A" \
  "$VM8_EXECUTABLE" run "$VALID_BINARY_PATH"

expect_success \
  "built-in trace" \
  "ADDR=0x11 OP=0x01 A=0x5A B=0x2A Z=0 C=0 NEXT=0x12 CYCLES=9 RESULT=halted" \
  "$VM8_EXECUTABLE" trace

expect_success \
  "binary trace" \
  "ADDR=0x11 OP=0x01 A=0x5A B=0x2A Z=0 C=0 NEXT=0x12 CYCLES=9 RESULT=halted" \
  "$VM8_EXECUTABLE" trace "$VALID_BINARY_PATH"

expect_failure \
  "missing binary" \
  "${MISSING_BINARY_PATH}: could not open binary input" \
  "$VM8_EXECUTABLE" run "$MISSING_BINARY_PATH"

: > "$EMPTY_BINARY_PATH"

expect_failure \
  "empty binary" \
  "${EMPTY_BINARY_PATH}: binary program is empty" \
  "$VM8_EXECUTABLE" run "$EMPTY_BINARY_PATH"

dd \
  if=/dev/zero \
  of="$OVERSIZED_BINARY_PATH" \
  bs="$OVERSIZED_BINARY_SIZE" \
  count=1 \
  status=none

expect_failure \
  "oversized binary" \
  "${OVERSIZED_BINARY_PATH}: binary input exceeds ${VM8_MEMORY_SIZE}-byte capacity" \
  "$VM8_EXECUTABLE" run "$OVERSIZED_BINARY_PATH"

printf '\xFF' > "$INVALID_OPCODE_BINARY_PATH"

expect_failure \
  "invalid opcode" \
  "Execution result: invalid opcode" \
  "$VM8_EXECUTABLE" run "$INVALID_OPCODE_BINARY_PATH"

printf '%s\n' "All VM8 process tests passed."
