#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool binary_writer_write(
    const char *output_path,
    const uint8_t *bytes,
    size_t byte_count
);
