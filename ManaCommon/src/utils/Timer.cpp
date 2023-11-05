#include "pch.h"
#include "utils/Timer.h"

using namespace std::chrono;

namespace Mana {

Timer::Timer() {
  Reset();
}

void Timer::Reset() {
  start_ = clock_type::now();
}

uint64_t Timer::GetMilliseconds() {
  clock_type::time_point now = clock_type::now();
  return duration_cast<milliseconds>(now - start_).count();
}

uint64_t Timer::GetMicroseconds() {
  clock_type::time_point now = clock_type::now();
  return duration_cast<microseconds>(now - start_).count();
}

// (now == end)
//std::chrono::duration<double> elapsed_seconds = end - start;
//std::time_t end_time = std::chrono::system_clock::to_time_t(end);
//std::cout << "finished computation at " << std::ctime(&end_time)
//    << "elapsed time: " << elapsed_seconds.count() << "s\n";

}  // namespace Mana
