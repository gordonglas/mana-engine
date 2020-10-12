#include "pch.h"
#include "base/com.h"
#include <objbase.h>

namespace Mana
{
	bool ComInitilizer::Init()
	{
		// uses COM STA
		//if (CoInitialize(nullptr) != S_OK)
		//    return false;

		// uses COM MTA
		if (CoInitializeEx(nullptr, COINIT_MULTITHREADED) != S_OK)
			return false;

		return true;
	}

	void ComInitilizer::Uninit()
	{
		CoUninitialize();
	}
}
