#include "timing.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>

namespace {
void test_clock_parser()
{
  std::uint32_t value = vm8::DEFAULT_CLOCK_HZ;
  for (const auto *invalid : {"", "0", "000", "-1", "+1", "1.5", "1 MHz",
      "1e6", "0x10", " 100", "100 ", "1,000", "1000000001", "4294967296",
      "18446744073709551616", "100\n"}) {
    assert(!vm8::parse_clock_hz(invalid, value));
    assert(value == vm8::DEFAULT_CLOCK_HZ);
  }
  assert(vm8::parse_clock_hz("1", value) && value == 1);
  assert(vm8::parse_clock_hz("1000000", value) && value == vm8::DEFAULT_CLOCK_HZ);
  assert(vm8::parse_clock_hz("1000000000", value) && value == vm8::MAX_CLOCK_HZ);
  assert(vm8::parse_clock_hz("0001000", value) && value == 1000);
}

void test_virtual_time()
{
  constexpr std::uint64_t POPCOUNT_CYCLES = 126;
  constexpr double TOLERANCE_SECONDS = 1e-12;
  assert(vm8::virtual_seconds(0, vm8::DEFAULT_CLOCK_HZ) == 0.0);
  assert(std::abs(vm8::virtual_seconds(POPCOUNT_CYCLES, vm8::DEFAULT_CLOCK_HZ) -
      126e-6) < TOLERANCE_SECONDS);
  assert(std::abs(vm8::virtual_seconds(POPCOUNT_CYCLES, 1000) - 0.126) < TOLERANCE_SECONDS);
  assert(vm8::virtual_seconds(POPCOUNT_CYCLES, 1) == 126.0);
  assert(vm8::virtual_seconds(std::numeric_limits<std::uint64_t>::max(), 1) > 1e19);
  assert(std::isnan(vm8::virtual_seconds(POPCOUNT_CYCLES, 0)));
  assert(vm8::format_seconds(0.0) == "0.000 s");
  assert(vm8::format_seconds(1e-9) == "1.000 ns");
  assert(vm8::format_seconds(1e-6) == "1.000 us");
  assert(vm8::format_seconds(126e-6) == "126.000 us");
  assert(vm8::format_seconds(1e-3) == "1.000 ms");
  assert(vm8::format_seconds(0.126) == "126.000 ms");
  assert(vm8::format_seconds(1.0) == "1.000 s");
}

void test_active_execution_timer()
{
  using namespace std::chrono_literals;
  const vm8::ExecutionTimer::TimePoint epoch{};
  vm8::ExecutionTimer timer;
  assert(!timer.running() && timer.seconds(epoch) == 0.0);
  timer.stop(epoch + 1s);
  assert(timer.seconds(epoch + 1s) == 0.0);
  timer.start(epoch + 2s);
  timer.start(epoch + 3s); // Starting twice must not discard the first second.
  assert(timer.running() && timer.seconds(epoch + 4s) == 2.0);
  timer.stop(epoch + 5s);
  timer.stop(epoch + 6s);
  assert(!timer.running() && timer.seconds(epoch + 100s) == 3.0);
  timer.start(epoch + 100s);
  assert(timer.seconds(epoch + 101s) == 4.0);
  timer.stop(epoch + 102s);
  assert(timer.seconds(epoch + 1000s) == 5.0); // Pauses never accumulate.
  timer.start(epoch + 1000s);
  timer.reset(); // Reset while running clears both accumulation and active state.
  assert(!timer.running() && timer.seconds(epoch + 2000s) == 0.0);
  timer.start(epoch + 2000s);
  timer.stop(epoch + 2000s + 250ms);
  assert(timer.seconds(epoch + 3000s) == 0.25);
  timer.reset();
  assert(timer.seconds(epoch + 3000s) == 0.0);
}
}

int main()
{
  test_clock_parser();
  test_virtual_time();
  test_active_execution_timer();
  std::puts("All Studio timing tests passed.");
}
