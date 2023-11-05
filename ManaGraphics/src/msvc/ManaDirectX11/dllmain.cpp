// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

#define EXPORT extern "C" __declspec(dllexport)

EXPORT int GetGlobalInt(int* pInt);

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

EXPORT int GetGlobalInt(int* pInt) {
  if (pInt) {
    return *pInt;
  }
  return 0;
}
