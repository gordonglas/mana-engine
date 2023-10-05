#include "pch.h"
#include "concurrency/Mutex.h"

#include <cassert>

namespace Mana {

Mutex::Mutex() {
  ::InitializeCriticalSection(&criticalSection_);
}

Mutex::~Mutex() {
  ::DeleteCriticalSection(&criticalSection_);
}

void Mutex::Lock() {
  ::EnterCriticalSection(&criticalSection_);
}

void Mutex::Unlock() {
  ::LeaveCriticalSection(&criticalSection_);
}

ScopedMutex::ScopedMutex(Mutex& mutex) : mutex_(mutex) {
  mutex_.Lock();
}

ScopedMutex::~ScopedMutex() {
  mutex_.Unlock();
}

}  // namespace Mana
