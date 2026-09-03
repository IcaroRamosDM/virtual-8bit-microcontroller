#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool binary_reader_read(
    const char *input_path,
    uint8_t *bytes,
    size_t byte_capacity,
    size_t *byte_count
);
