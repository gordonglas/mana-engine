#pragma once

#include "ManaGlobals.h"
#include <cstdio>
#include <cstdarg>
#include <cstring>

namespace Mana
{
	enum class Verbosity : U32
	{
		Error = 0,
		Warning = 1,
		Info = 2,
		Verbose = 3
	};

	enum class Channel : U32
	{
		All = 0,
		Init = 1,
		Shutdown = 2,
		Graphics = 3,
		Sound = 4
	};

	// use the macros below instead of these functions
	bool LogInit(const char* logFile);
	void LogError(Channel channel, bool newline, const xchar* format, ...);
	void LogWarning(Channel channel, bool newline, const xchar* format, ...);
	void LogInfo(Channel channel, bool newline, const xchar* format, ...);
	void LogVerbose(Channel channel, bool newline, const xchar* format, ...);
}

#ifdef MANA_LOGGING_ENABLED

#define ManaLogInit(file)						::Mana::LogInit(file)

#define ManaLogError(channel, format, ...)		::Mana::LogError(channel, false, format, __VA_ARGS__)
#define ManaLogWarning(channel, format, ...)	::Mana::LogWarning(channel, false, format, __VA_ARGS__)
#define ManaLogInfo(channel, format, ...)		::Mana::LogInfo(channel, false, format, __VA_ARGS__)
#define ManaLogVerbose(channel, format, ...)	::Mana::LogVerbose(channel, false, format, __VA_ARGS__)

#define ManaLogLnError(channel, format, ...)	::Mana::LogError(channel, true, format, __VA_ARGS__)
#define ManaLogLnWarning(channel, format, ...)	::Mana::LogWarning(channel, true, format, __VA_ARGS__)
#define ManaLogLnInfo(channel, format, ...)		::Mana::LogInfo(channel, true, format, __VA_ARGS__)
#define ManaLogLnVerbose(channel, format, ...)	::Mana::LogVerbose(channel, true, format, __VA_ARGS__)

#else

#define ManaLogInit(file)

#define ManaLogError(channel, format, ...)
#define ManaLogWarning(channel, format, ...)
#define ManaLogInfo(channel, format, ...)
#define ManaLogVerbose(channel, format, ...)

#define ManaLogLnError(channel, format, ...)
#define ManaLogLnWarning(channel, format, ...)
#define ManaLogLnInfo(channel, format, ...)
#define ManaLogLnVerbose(channel, format, ...)

#endif
