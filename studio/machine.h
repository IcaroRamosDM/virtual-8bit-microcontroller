#pragma once

#include <array>
#include <string>

extern "C" {
#include "assembly.h"
#include "byte_value.h"
#include "cpu.h"
}

namespace vm8 {
constexpr unsigned RUN_INSTRUCTION_LIMIT = 10000;

class Machine {
public:
  Cpu cpu{};
  AssemblyImage image{};
  std::array<bool, CPU_MEMORY_SIZE> breakpoints{};
  std::string source;
  std::string messages;
  bool assembled = false;

  bool assemble(const std::string &text, uint8_t input);
  void reset(uint8_t input);
  CpuStepResult step(std::string *trace = nullptr);
  std::string memory_text() const;
  std::string binary_text() const;
  static const char *result_name(CpuStepResult result);
};

int self_test();
}
