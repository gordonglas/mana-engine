#pragma once

#include "mana-globals.h"
#include "target/target-os.h"

namespace Mana
{
	class CriticalSection
	{
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


	class ScopedCriticalSection
	{
	public:
		ScopedCriticalSection(CriticalSection& cs) : m_cs(cs)
		{
			m_cs.Lock();
		}
		~ScopedCriticalSection()
		{
			m_cs.Unlock();
		}

		ScopedCriticalSection(const ScopedCriticalSection&) = delete;
		ScopedCriticalSection& operator=(const ScopedCriticalSection&) = delete;
	private:
		CriticalSection& m_cs;
	};
}
