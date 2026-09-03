#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "cpu.h"

enum
{
  TEST_A_VALUE = 0x2A,
  EXPECTED_OBSERVATION_COUNT = 2,
  LOAD_INSTRUCTION_ADDRESS = 0,
  HALT_INSTRUCTION_ADDRESS = 2
};

typedef struct StepObservation
{
  uint8_t instruction_address;
  uint8_t opcode;
  uint8_t register_a;
  uint8_t next_program_counter;
  uint64_t cycle_count;
  CpuStepResult step_result;
} StepObservation;

typedef struct ObservationLog
{
  StepObservation entries[EXPECTED_OBSERVATION_COUNT];
  size_t count;
} ObservationLog;

static void record_step(
    uint8_t instruction_address,
    uint8_t opcode,
    const Cpu *cpu,
    CpuStepResult step_result,
    void *context
)
{
  assert(cpu != NULL);
  assert(context != NULL);

  ObservationLog *const log = context;

  assert(log->count < EXPECTED_OBSERVATION_COUNT);

  StepObservation *const entry =
    &log->entries[log->count];

  *entry = (StepObservation){
    .instruction_address = instruction_address,
    .opcode = opcode,
    .register_a = cpu->register_a,
    .next_program_counter = cpu->program_counter,
    .cycle_count = cpu->cycle_count,
    .step_result = step_result
  };

  ++log->count;
}

static void test_observes_each_executed_instruction(void)
{
  static const uint8_t program_bytes[] =
  {
    OPCODE_LOAD_IMMEDIATE_A,
    TEST_A_VALUE,
    OPCODE_HALT
  };

  const uint64_t instruction_limit =
    EXPECTED_OBSERVATION_COUNT;

  Cpu cpu = {0};
  ObservationLog log = {0};

  const bool program_loaded = cpu_load_program(
    &cpu,
    program_bytes,
    sizeof program_bytes
  );

  assert(program_loaded);

  const CpuRunResult run_result =
    cpu_run_with_observer(
      &cpu,
      instruction_limit,
      record_step,
      &log
    );

  assert(run_result == CPU_RUN_HALTED);
  assert(log.count == EXPECTED_OBSERVATION_COUNT);

  const StepObservation *const load_observation =
    &log.entries[0];

  assert(
    load_observation->instruction_address ==
    LOAD_INSTRUCTION_ADDRESS
  );

  assert(
    load_observation->opcode ==
    OPCODE_LOAD_IMMEDIATE_A
  );

  assert(load_observation->register_a == TEST_A_VALUE);

  assert(
    load_observation->next_program_counter ==
    HALT_INSTRUCTION_ADDRESS
  );

  assert(load_observation->cycle_count == UINT64_C(1));
  assert(load_observation->step_result == CPU_STEP_OK);

  const StepObservation *const halt_observation =
    &log.entries[1];

  assert(
    halt_observation->instruction_address ==
    HALT_INSTRUCTION_ADDRESS
  );

  assert(halt_observation->opcode == OPCODE_HALT);
  assert(halt_observation->register_a == TEST_A_VALUE);

  assert(
    halt_observation->next_program_counter ==
    sizeof program_bytes
  );

  assert(halt_observation->cycle_count == UINT64_C(2));

  assert(
    halt_observation->step_result ==
    CPU_STEP_HALTED
  );
}

int main(void)
{
  test_observes_each_executed_instruction();

  puts("All CPU-observer tests passed.");

  return EXIT_SUCCESS;
}
