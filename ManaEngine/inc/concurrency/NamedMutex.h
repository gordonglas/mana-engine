#pragma once

#include "ManaGlobals.h"
#include "target/TargetOS.h"

namespace Mana {

// A named cross-process mutex.
// If you only need a cross-thread mutex, use Mutex instead.
class ScopedNamedMutex {
 public:
  // https://learn.microsoft.com/en-us/windows/win32/sync/object-names
  ScopedNamedMutex(xstring name);
  ~ScopedNamedMutex();
  // Doesn't block.
  // returns true if this thread has ownership,
  // returns false if some process/thread already has ownership.
  bool TryLock();

  ScopedNamedMutex(const ScopedNamedMutex&) = delete;
  ScopedNamedMutex& operator=(const ScopedNamedMutex&) = delete;

 protected:
  xstring name_;
#ifdef OS_WIN
  HANDLE mutex_ = nullptr;
#endif
  void Unlock();
  void Close();
};

}  // namespace Mana
