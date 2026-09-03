#pragma once

#include <stdint.h>
#include <stdio.h>

#include "cpu.h"

void cpu_trace_print_header(FILE *output);

void cpu_trace_observer(
    uint8_t instruction_address,
    uint8_t opcode,
    const Cpu *cpu,
    CpuStepResult step_result,
    void *context
);
