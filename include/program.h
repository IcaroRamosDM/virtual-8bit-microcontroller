#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct Program
{
  const uint8_t *bytes;
  size_t size;
} Program;

Program program_get_demo(void);
