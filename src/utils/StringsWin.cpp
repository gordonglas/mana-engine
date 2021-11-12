#include "pch.h"
#include "target/TargetOS.h"
#include "utils/Strings.h"
#include <fstream>

namespace Mana
{
	int Utf16ToUtf8(char* output, size_t outputSize, const wchar_t* wide)
	{
		// TODO: wcsnlen?
		int wideCharLen = (int)std::wcslen(wide) + 1;
		// is buf size big enough
		int utf8CharLen = ::WideCharToMultiByte(CP_UTF8, 0, wide,
			wideCharLen, nullptr, 0, nullptr, nullptr);
		if (utf8CharLen > (int)outputSize)
			return -1;

		utf8CharLen = ::WideCharToMultiByte(CP_UTF8, 0, wide, wideCharLen,
			output, (int)outputSize, nullptr, nullptr);
		if (utf8CharLen == 0)
			return -1;

		return utf8CharLen;
	}

	int Utf8ToUtf16(wchar_t* output, size_t outputSize, const char* utf8)
	{
		// TODO: strnlen?
		int utf8CharLen = (int)std::strlen(utf8) + 1;
		// is buf size big enough
		int wideCharLen = ::MultiByteToWideChar(CP_UTF8, 0, utf8,
			utf8CharLen, nullptr, 0);
		if (wideCharLen > (int)outputSize)
			return -1;

		wideCharLen = ::MultiByteToWideChar(CP_UTF8, 0, utf8, utf8CharLen,
			output, (int)outputSize);
		if (wideCharLen == 0)
			return -1;

		return wideCharLen;
	}

	std::string Utf16ToUtf8(std::wstring wide)
	{
		size_t utf8BufLen = wide.length() * 4 + 1;
		char* output = new char[utf8BufLen];
		if (!output)
		{
			return "";
		}

		int len = Utf16ToUtf8(output, utf8BufLen, wide.c_str());
		if (len == -1)
		{
			delete[] output;
			return "";
		}

		std::string ret(output);
		delete[] output;
		return ret;
	}

	std::wstring Utf8ToUtf16(std::string utf8)
	{
		size_t utf16BufLen = utf8.length() * 4 + 1;
		wchar_t* output = new wchar_t[utf16BufLen];
		if (!output)
		{
			return L"";
		}

		int len = Utf8ToUtf16(output, utf16BufLen, utf8.c_str());
		if (len == -1)
		{
			delete[] output;
			return L"";
		}

		std::wstring ret(output);
		delete[] output;
		return ret;
	}
}