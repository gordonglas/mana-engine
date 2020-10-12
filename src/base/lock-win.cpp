#include "pch.h"
#include "base/lock.h"

namespace Mana
{
	CriticalSection::CriticalSection()
	{
		InitializeCriticalSection(&m_cs);
	}

	CriticalSection::~CriticalSection()
	{
		DeleteCriticalSection(&m_cs);
	}

	void CriticalSection::Lock()
	{
		EnterCriticalSection(&m_cs);
	}

	void CriticalSection::Unlock()
	{
		LeaveCriticalSection(&m_cs);
	}
}
