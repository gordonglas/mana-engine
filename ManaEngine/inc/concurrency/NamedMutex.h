#pragma once

#include "ManaGlobals.h"
#include "target/TargetOS.h"

namespace Mana {

// An named cross-process mutex.
// If you need a cross-thread mutex, use Mutex instead.
class NamedMutex {
 public:
  NamedMutex(const xstring& name);
  ~NamedMutex();
  void Lock();
  void Unlock();

  NamedMutex(const NamedMutex&) = delete;
  NamedMutex& operator=(const NamedMutex&) = delete;

 protected:
#ifdef OS_WIN
  HANDLE mutex_;
#endif
};

class ScopedNamedMutex {
 public:
  ScopedNamedMutex(NamedMutex& mutex);
  ~ScopedNamedMutex();

  ScopedNamedMutex(const ScopedNamedMutex&) = delete;
  ScopedNamedMutex& operator=(const ScopedNamedMutex&) = delete;

 private:
  NamedMutex& mutex_;
};

}  // namespace Mana
