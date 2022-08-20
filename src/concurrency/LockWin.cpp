#include "pch.h"
#include "concurrency/Lock.h"

#include <cassert>

namespace Mana {

CriticalSection::CriticalSection() {
  ::InitializeCriticalSection(&m_cs);
}

CriticalSection::~CriticalSection() {
  ::DeleteCriticalSection(&m_cs);
}

void CriticalSection::Lock() {
  ::EnterCriticalSection(&m_cs);
}

void CriticalSection::Unlock() {
  ::LeaveCriticalSection(&m_cs);
}

ScopedCriticalSection::ScopedCriticalSection(CriticalSection& cs) : m_cs(cs) {
  m_cs.Lock();
}

ScopedCriticalSection::~ScopedCriticalSection() {
  m_cs.Unlock();
}

Mutex::Mutex() {
  mutex_ = ::CreateMutexW(nullptr,   // default security attributes
                        FALSE,     // initially not owned
                        nullptr);  // unnamed mutex
}

Mutex::~Mutex() {
  ::CloseHandle(mutex_);
}

void Mutex::Lock() {
  DWORD dwWaitResult = WaitForSingleObject(mutex_, INFINITE);
  assert(dwWaitResult == WAIT_OBJECT_0);
}

void Mutex::Unlock() {
  ReleaseMutex(mutex_);
}

ScopedMutex::ScopedMutex(Mutex& mutex) : mutex_(mutex) {
  mutex_.Lock();
}

ScopedMutex::~ScopedMutex() {
  mutex_.Unlock();
}

}  // namespace Mana
