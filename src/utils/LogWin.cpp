#include "pch.h"
#include "utils/Log.h"
#include "target/TargetOS.h"
#include "utils/Strings.h"
#include "utils/Lock.h"
#include <cwchar> // _vscwprintf, vswprintf
#include <string>

namespace Mana {
char logFile[20];
CriticalSection logFileLock;
bool initialized = false;

static void Log(Verbosity verbosity,
                Channel channel,
                bool newline,
                const wchar_t* format,
                va_list args);
static const wchar_t* GetVerbosityString(Verbosity verbosity);
static const wchar_t* GetChannelString(Channel channel);
static std::wstring GetTimeString();
//static int GetTimeString(wchar_t* buf, size_t bufSize);

bool LogInit(const char* file) {
  strncpy_s(logFile, 20, file, 20);
  FILE* pFile = nullptr;
  initialized = fopen_s(&pFile, logFile, "w") == 0 && pFile;
  if (initialized && pFile) {
    std::fclose(pFile);
  }
  return initialized;
}

void LogError(Channel channel, bool newline, const xchar* format, ...) {
  if (!initialized)
    return;

  va_list args;
  va_start(args, format);
  Log(Verbosity::Error, channel, newline, format, args);
  va_end(args);
}

void LogWarning(Channel channel, bool newline, const xchar* format, ...) {
  if (!initialized)
    return;

  va_list args;
  va_start(args, format);
  Log(Verbosity::Warning, channel, newline, format, args);
  va_end(args);
}

void LogInfo(Channel channel, bool newline, const xchar* format, ...) {
  if (!initialized)
    return;

  va_list args;
  va_start(args, format);
  Log(Verbosity::Info, channel, newline, format, args);
  va_end(args);
}

void LogVerbose(Channel channel, bool newline, const xchar* format, ...) {
  if (!initialized)
    return;

  va_list args;
  va_start(args, format);
  Log(Verbosity::Verbose, channel, newline, format, args);
  va_end(args);
}

static void Log(Verbosity verbosity,
                Channel channel,
                bool newline,
                const wchar_t* format,
                va_list args) {
  // Format: [hh:mm:ss.fff]: [VERBOSITY] [CHANNEL]: Message

  std::wstring wtime = GetTimeString();
  // wchar_t bufTime[20];
  // GetTimeString(bufTime, 20);

  wchar_t bufStart[80];
  int written =
      std::swprintf(bufStart, 80, L"[%s] %s %s: ", wtime.c_str(),
                    GetVerbosityString(verbosity), GetChannelString(channel));

  std::wstring wstr(bufStart);

  // get size of buf needed
  int len = _vscwprintf(format, args);
  if (len > 0) {
    wchar_t* buf = new wchar_t[len + 1];

    /*
    On success, the total number of characters written is returned.
    This count does not include the additional null-character automatically
    appended at the end of the string. A negative number is returned on failure,
    including when the resulting string to be written to ws would be longer
    than n characters.
    */
    written = std::vswprintf(buf, len + 1, format, args);
    if (written < 0) {
      // buf too small
      delete[] buf;
      return;
    }

    wstr.append(buf);
    delete[] buf;
  }

  if (newline) {
    wstr.append(L"\n");
  }

  OutputDebugStringW(wstr.c_str());

  std::string utf8 = Utf16ToUtf8(wstr);

  {
    ScopedCriticalSection slock(logFileLock);
    FILE* pFile = nullptr;
    if (fopen_s(&pFile, Mana::logFile, "ab") == 0 && pFile) {
      std::fwrite(utf8.c_str(), 1, utf8.length(), pFile);
      std::fclose(pFile);
    }
  }
}

static const wchar_t* GetVerbosityString(Verbosity verbosity) {
  switch (verbosity) {
    case Mana::Verbosity::Error:
      return L"[ERROR]";
    case Mana::Verbosity::Warning:
      return L"[WARNING]";
    case Mana::Verbosity::Info:
      return L"[INFO]";
    case Mana::Verbosity::Verbose:
      return L"[VERBOSE]";
    default:
      return L"[MISSING VERBOSITY]";
  }
}

static const wchar_t* GetChannelString(Channel channel) {
  switch (channel) {
    case Mana::Channel::All:
      return L"[ALL]";
    case Mana::Channel::Init:
      return L"[INIT]";
    case Mana::Channel::Shutdown:
      return L"[SHUTDOWN]";
    case Mana::Channel::Graphics:
      return L"[GRAPHICS]";
    case Mana::Channel::Sound:
      return L"[SOUND]";
    default:
      return L"[MISSING CHANNEL]";
  }
}

// output in form of "hh:mm:ss.fff"
static std::wstring GetTimeString() {
  SYSTEMTIME time;
  GetLocalTime(&time);  // local time. Use GetSystemTime(&time) for UTC.

  wchar_t bufTime[20];
  int written = swprintf(bufTime, 20, L"%02d:%02d:%02d.%03d", time.wHour,
                         time.wMinute, time.wSecond, time.wMilliseconds);

  std::wstring ret;
  if (written > 0) {
    ret.append(bufTime);
  }

  return ret;
}

// output in form of "hh:mm:ss.fff"
//static int GetTimeString(wchar_t* buf, size_t bufSize)
//{
//	int ret = 0;
//
//	SYSTEMTIME time;
//	GetLocalTime(&time); // local time. Use GetSystemTime(&time) for UTC.
//	ret = swprintf(buf, bufSize, L"%02d:%02d:%02d.%03d",
//		time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
//
//	return ret;
//}
}  // namespace Mana
