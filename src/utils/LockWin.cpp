#include "pch.h"
#include "utils/Lock.h"

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
}  // namespace Mana
