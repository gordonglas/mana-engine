#pragma once

#include <chrono>

namespace Mana {

//typedef std::chrono::steady_clock clock_type;
typedef std::chrono::high_resolution_clock clock_type;

class Timer {
 public:
  Timer();
  virtual ~Timer() = default;

  Timer(const Timer&) = delete;
  Timer& operator=(const Timer&) = delete;

  void Reset();
  // milliseconds since init or last reset
  uint64_t GetMilliseconds();
  // microseconds since init or last reset
  uint64_t GetMicroseconds();

 private:
  clock_type::time_point start_;
};

}  // namespace Mana
