#include "timing.h"
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>

namespace vm8 {
bool parse_clock_hz(std::string_view text, std::uint32_t &value)
{
  if (text.empty()) return false;
  constexpr std::uint32_t DECIMAL_BASE = 10;
  std::uint32_t candidate = 0;
  for (const char character : text) {
    if (character < '0' || character > '9') return false;
    const auto digit = static_cast<std::uint32_t>(character - '0');
    if (candidate > (MAX_CLOCK_HZ - digit) / DECIMAL_BASE) return false;
    candidate = candidate * DECIMAL_BASE + digit;
  }
  if (candidate == 0) return false;
  value = candidate;
  return true;
}

double virtual_seconds(std::uint64_t cycles, std::uint32_t clock_hz)
{
  if (clock_hz == 0) return std::numeric_limits<double>::quiet_NaN();
  // Divide in floating point; multiplying a uint64_t cycle count can overflow.
  return static_cast<double>(cycles) / clock_hz;
}

std::string format_seconds(double seconds)
{
  constexpr double MILLISECOND = 1e-3, MICROSECOND = 1e-6, NANOSECOND = 1e-9;
  double unit = 1.0;
  const char *suffix = "s";
  if (seconds > 0.0 && seconds < MICROSECOND) { unit = NANOSECOND; suffix = "ns"; }
  else if (seconds > 0.0 && seconds < MILLISECOND) { unit = MICROSECOND; suffix = "us"; }
  else if (seconds > 0.0 && seconds < 1.0) { unit = MILLISECOND; suffix = "ms"; }
  std::ostringstream text;
  text.imbue(std::locale::classic());
  text << std::fixed << std::setprecision(3) << seconds / unit << ' ' << suffix;
  return text.str();
}

void ExecutionTimer::start(TimePoint now)
{
  if (running_) return;
  started_ = now;
  running_ = true;
}

void ExecutionTimer::stop(TimePoint now)
{
  if (!running_) return;
  elapsed_ += now - started_;
  running_ = false;
}

void ExecutionTimer::reset()
{
  elapsed_ = Clock::duration::zero();
  started_ = TimePoint{};
  running_ = false;
}

double ExecutionTimer::seconds(TimePoint now) const
{
  return std::chrono::duration<double>(elapsed_ +
      (running_ ? now - started_ : Clock::duration::zero())).count();
}
}
