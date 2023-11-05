#pragma once

#include "ManaGlobals.h"
#include "target/TargetOS.h"

namespace Mana {

// An unnamed cross-thread mutex.
// If you need a cross-process mutex, use NamedMutex instead.
class Mutex {
 public:
  Mutex();
  ~Mutex();
  void Lock();
  void Unlock();

  Mutex(const Mutex&) = delete;
  Mutex& operator=(const Mutex&) = delete;

 protected:
#ifdef OS_WIN
  // CRITICAL_SECTION might be more efficient than std::mutex on Windows.
  // TODO(gglas): See how std::mutex is implemented on Windows.
  //              Test CRITICAL_SECTION against std::mutex.
  CRITICAL_SECTION criticalSection_;
#else
  std::mutex mutex_;
#endif
};

class ScopedMutex {
 public:
  ScopedMutex(Mutex& mutex);
  ~ScopedMutex();

  ScopedMutex(const ScopedMutex&) = delete;
  ScopedMutex& operator=(const ScopedMutex&) = delete;

 private:
  Mutex& mutex_;
};

}  // namespace Mana
