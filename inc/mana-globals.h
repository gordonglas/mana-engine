#pragma once

// disable STL exceptions (must also disable exceptions in project properties)
// C/C++ -> Code Generation -> Enable C++ Exceptions
#define _HAS_EXCEPTIONS 0

//#define _CRT_SECURE_NO_WARNINGS

// define our own OS/archtecture macros
#if _WIN32 || _WIN64
#define OS_WIN
#endif

#ifdef OS_WIN

// SSE intrisics (__m128 etc)
// http://felix.abecassis.me/2011/09/cpp-getting-started-with-sse/
#include <xmmintrin.h>

namespace Mana {
	// portable data types
	typedef unsigned char		U8;
	typedef signed char			I8;
	typedef unsigned short		U16;
	typedef short				I16;
	typedef unsigned int		U32;
	typedef int					I32;
	typedef unsigned long long	U64;
	typedef long long			I64;
	typedef float				F32;
	typedef __m128				VF32;
}

#endif

#if defined(_DEBUG)
#define MANA_LOGGING_ENABLED 1
#endif
#include "base/Log.h"
