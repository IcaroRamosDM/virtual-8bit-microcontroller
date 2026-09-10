#pragma once

#include <stdarg.h>
#include <stdio.h>

typedef void (*AssemblerMessageHandler)(const char *message, void *context);

typedef struct AssemblerDiagnostics
{
  AssemblerMessageHandler handler;
  void *context;
} AssemblerDiagnostics;

/* A missing handler preserves the command-line stderr diagnostics. */
static inline void assembler_report(
    const AssemblerDiagnostics *diagnostics,
    const char *format,
    ...)
{
  va_list arguments;
  va_start(arguments, format);

  if (diagnostics == NULL || diagnostics->handler == NULL)
  {
    (void)vfprintf(stderr, format, arguments);
  }
  else
  {
    enum { ASSEMBLER_MESSAGE_CAPACITY = 2048 };
    char message[ASSEMBLER_MESSAGE_CAPACITY];
    (void)vsnprintf(message, sizeof message, format, arguments);
    diagnostics->handler(message, diagnostics->context);
  }

  va_end(arguments);
}
