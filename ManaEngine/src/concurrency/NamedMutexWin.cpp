#include "pch.h"
#include "concurrency/NamedMutex.h"

#include <cassert>

namespace Mana {

ScopedNamedMutex::ScopedNamedMutex(xstring name) : name_(name) {
  assert(!name.empty());
}

ScopedNamedMutex::~ScopedNamedMutex() {
  Unlock();
  Close();
}

bool ScopedNamedMutex::TryLock() {
  mutex_ = ::CreateMutexW(nullptr,         // default security attributes
                          TRUE,            // initially locked
                          name_.c_str());  // named mutex

  if (!mutex_ || GetLastError() == ERROR_ALREADY_EXISTS) {
    Unlock();
    Close();
    return false;
  }

  return true;
}

void ScopedNamedMutex::Unlock() {
  if (mutex_) {
    ::ReleaseMutex(mutex_);
  }
}

void ScopedNamedMutex::Close() {
  if (mutex_) {
    ::CloseHandle(mutex_);
    mutex_ = nullptr;
  }
}

}  // namespace Mana
