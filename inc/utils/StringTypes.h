// Cross-platform string types.
//   For unicode - xchar, xstring        - On Windows: UTF-16, Else UTF-8
//   For unicode literal: _X(""), _X('') - On Windows: UTF-16, Else UTF-8

#pragma once

#include <string>
#include "target/OSDefines.h"

#ifdef OS_WIN
	typedef wchar_t xchar;
	namespace Mana {
		typedef std::wstring xstring;
	}
	#define _X(x) L ##x
#else
	typedef char xchar;
	namespace Mana {
		typedef std::string xstring;
	}
	#define _X(x) x
#endif
