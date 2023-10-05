#include "pch.h"
#include "concurrency/NamedMutex.h"

#include <cassert>

namespace Mana {

NamedMutex::NamedMutex(const xstring& name) {
  assert(!name.empty());
  mutex_ = ::CreateMutexW(nullptr,        // default security attributes
                          FALSE,          // initially unlocked
                          name.c_str());  // named mutex
}

NamedMutex::~NamedMutex() {
  ::CloseHandle(mutex_);
}

void NamedMutex::Lock() {
  DWORD dwWaitResult = ::WaitForSingleObject(mutex_, INFINITE);
  assert(dwWaitResult == WAIT_OBJECT_0);
}

void NamedMutex::Unlock() {
  ::ReleaseMutex(mutex_);
}

ScopedNamedMutex::ScopedNamedMutex(NamedMutex& mutex) : mutex_(mutex) {
  mutex_.Lock();
}

ScopedNamedMutex::~ScopedNamedMutex() {
  mutex_.Unlock();
}

}  // namespace Mana
