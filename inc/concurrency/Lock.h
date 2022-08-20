#pragma once

#include "ManaGlobals.h"
#include "target/TargetOS.h"

namespace Mana {

class CriticalSection {
 public:
  CriticalSection();
  ~CriticalSection();
  void Lock();
  void Unlock();

  CriticalSection(const CriticalSection&) = delete;
  CriticalSection& operator=(const CriticalSection&) = delete;

 protected:
#ifdef OS_WIN
  mutable CRITICAL_SECTION m_cs;
#endif
};

class ScopedCriticalSection {
 public:
  ScopedCriticalSection(CriticalSection& cs);
  ~ScopedCriticalSection();

  ScopedCriticalSection(const ScopedCriticalSection&) = delete;
  ScopedCriticalSection& operator=(const ScopedCriticalSection&) = delete;

 private:
  CriticalSection& m_cs;
};

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
  HANDLE mutex_;
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
