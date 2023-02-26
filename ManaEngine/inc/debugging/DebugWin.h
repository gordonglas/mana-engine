#pragma once

#include "ManaGlobals.h"
#include "target/TargetOS.h"

namespace Mana {

#ifdef OS_WIN
  // call this between BeginPaint and EndPaint in WM_PAINT.
void Debug_DrawText_GDI(HWND hWnd,
                        HDC hdc,
                        LONG x,
                        LONG y,
                        const std::wstring& text);
#endif

} // namespace Mana
