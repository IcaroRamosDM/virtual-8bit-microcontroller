#pragma once

#include <stdbool.h>
#include <stdio.h>

#include "cpu.h"
#include "program.h"

bool monitor_run(
    Cpu *cpu,
    Program program,
    FILE *input,
    FILE *output,
    FILE *error_output
);
