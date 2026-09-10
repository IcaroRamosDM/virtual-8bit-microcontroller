#include "machine.h"
#include "assets.h"

#include <cstdio>
#include <cstring>
#include <sstream>

namespace vm8 {
static void collect(const char *message, void *context)
{
  static_cast<std::string *>(context)->append(message);
}

bool Machine::assemble(const std::string &text, uint8_t input)
{
  messages.clear();
  AssemblyImage candidate{};
  const AssemblerDiagnostics diagnostics{collect, &messages};
  assembled = assembly_compile(text.c_str(), &candidate, diagnostics);
  if (!assembled) {
    image.error_line = candidate.error_line;
    return false;
  }
  if (source != text) breakpoints.fill(false);
  source = text;
  image = candidate;
  reset(input);
  return true;
}

void Machine::reset(uint8_t input)
{
  cpu_reset(&cpu);
  if (assembled) (void)cpu_load_program(&cpu, image.bytes, image.byte_count);
  cpu_set_input_port(&cpu, input);
}

const char *Machine::result_name(CpuStepResult result)
{
  switch (result) {
    case CPU_STEP_OK: return "ok";
    case CPU_STEP_HALTED: return "halted";
    case CPU_STEP_INVALID_OPCODE: return "invalid opcode";
    case CPU_STEP_STACK_OVERFLOW: return "stack overflow";
    case CPU_STEP_STACK_UNDERFLOW: return "stack underflow";
  }
  return "unknown result";
}

CpuStepResult Machine::step(std::string *trace)
{
  const uint8_t address = cpu.program_counter;
  const uint8_t opcode = cpu_read_memory(&cpu, address);
  const auto *metadata = instruction_set_find_by_opcode(opcode);
  const CpuStepResult result = cpu_step(&cpu);
  if (trace) {
    char line[320];
    std::snprintf(line, sizeof line,
        "%04llu  %02X  %-7s A=%02X B=%02X SP=%02X Z=%d C=%d IN=%02X OUT=%02X  %s\n",
        static_cast<unsigned long long>(cpu.cycle_count), static_cast<unsigned>(address),
        metadata ? metadata->mnemonic : "UNKNOWN", static_cast<unsigned>(cpu.register_a),
        static_cast<unsigned>(cpu.register_b), static_cast<unsigned>(cpu.stack_pointer),
        cpu.zero_flag, cpu.carry_flag, static_cast<unsigned>(cpu_read_memory(&cpu, CPU_INPUT_PORT_ADDRESS)),
        static_cast<unsigned>(cpu_get_output_port(&cpu)), result_name(result));
    *trace = line;
  }
  return result;
}

std::string Machine::memory_text() const
{
  std::string text = "      00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n\n";
  constexpr unsigned ROW_BYTES = 16;
  for (unsigned address = 0; address < CPU_MEMORY_SIZE; address += ROW_BYTES) {
    char row[96];
    int used = std::snprintf(row, sizeof row, "%02X    ", address);
    for (unsigned col = 0; col < ROW_BYTES; ++col) {
      used += std::snprintf(row + used, sizeof row - static_cast<size_t>(used), "%02X ",
          static_cast<unsigned>(cpu_read_memory(&cpu, static_cast<uint8_t>(address + col))));
    }
    text += row;
    text += '\n';
  }
  text += "\n00-ED  Program and data (238 bytes)\nEE     Input port (read-only to CPU)\nEF     Output port\nF0-FF  Stack (16 bytes, grows downward)\n";
  return text;
}

std::string Machine::binary_text() const
{
  std::string text = "ADDR  BYTE  SOURCE LINE  KIND\n\n";
  for (size_t i = 0; i < image.byte_count; ++i) {
    char row[128];
    const auto *metadata = image.instruction_start[i]
        ? instruction_set_find_by_opcode(image.bytes[i]) : nullptr;
    std::snprintf(row, sizeof row, "%02X    %02X    %5u        %s\n",
        static_cast<unsigned>(i), static_cast<unsigned>(image.bytes[i]),
        static_cast<unsigned>(image.line_for_address[i]), metadata ? metadata->mnemonic : "operand / data");
    text += row;
  }
  return text;
}

int self_test()
{
  Machine machine;
  for (const unsigned input : {0U, 0xA5U, 0xFFU}) {
    if (!machine.assemble(popcount_source, static_cast<uint8_t>(input))) return 1;
    unsigned expected = input == 0 ? 0 : input == 0xA5 ? 4 : 8;
    unsigned steps = 0;
    while (!machine.cpu.halted && steps++ < RUN_INSTRUCTION_LIMIT) machine.step();
    if (!machine.cpu.halted || cpu_get_output_port(&machine.cpu) != expected ||
        machine.cpu.stack_pointer != CPU_STACK_EMPTY_POINTER ||
        machine.cpu.cycle_count != 114 + 3 * expected) return 2;
  }
  if (!machine.assemble(demo_source, 0)) return 3;
  for (unsigned steps = 0; !machine.cpu.halted && steps < RUN_INSTRUCTION_LIMIT; ++steps) machine.step();
  if (machine.cpu.register_a != 0x5A || machine.cpu.cycle_count != 9) return 4;
  if (machine.assemble("HALT\nLDI A 1\n", 0) || machine.image.error_line != 2 ||
      machine.messages.find("comma") == std::string::npos) return 5;
  if (machine.assemble("; empty", 0)) return 6;
  for (unsigned opcode = 0; opcode < CPU_MEMORY_SIZE; ++opcode) {
    const auto *info = instruction_set_find_by_opcode(static_cast<uint8_t>(opcode));
    if (info && std::strstr(help_reference, info->mnemonic) == nullptr) return 7;
  }
  std::puts("VM8 Studio backend and embedded assets: all checks passed.");
  return 0;
}
}
