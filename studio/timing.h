#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>

namespace vm8 {
constexpr std::uint32_t DEFAULT_CLOCK_HZ = 1000000;
constexpr std::uint32_t MAX_CLOCK_HZ = 1000000000;

// Decimal Hz only. Invalid input leaves the caller's value unchanged.
bool parse_clock_hz(std::string_view text, std::uint32_t &value);
double virtual_seconds(std::uint64_t cycles, std::uint32_t clock_hz);
std::string format_seconds(double seconds);

class ExecutionTimer {
public:
  using Clock = std::chrono::steady_clock;
  using TimePoint = Clock::time_point;

  // Explicit timestamps make pause/resume tests deterministic, without sleeps.
  void start(TimePoint now = Clock::now());
  void stop(TimePoint now = Clock::now());
  void reset();
  double seconds(TimePoint now = Clock::now()) const;
  bool running() const { return running_; }

private:
  TimePoint started_{};
  Clock::duration elapsed_{};
  bool running_ = false;
};
}
