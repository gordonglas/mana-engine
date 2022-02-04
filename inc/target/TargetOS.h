#pragma once

#if _WIN32 || _WIN64

// Including SDKDDKVer.h defines the highest available Windows platform.
// If you wish to build your application for a previous Windows platform,
// include WinSDKVer.h and set the _WIN32_WINNT macro to the platform you wish
// to support before including SDKDDKVer.h.

// Support Windows 10 and up
#define _WIN32_WINNT 0x0A00
#include <SDKDDKVer.h>

// Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN
// Windows Header Files
#include <windows.h>

#endif
