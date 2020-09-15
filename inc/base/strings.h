#pragma once

#include <string>

namespace Mana
{
	// TODO: test these!
	std::string Utf16ToUtf8(std::wstring wide);
	std::wstring Utf8ToUtf16(std::string utf8);
}
