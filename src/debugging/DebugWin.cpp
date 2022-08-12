#include "pch.h"
#include "debugging/DebugWin.h"

namespace Mana {

#ifdef OS_WIN
void Debug_DrawText_GDI(HWND hWnd,
                        HDC hdc,
                        LONG x,
                        LONG y,
                        const std::wstring& text) {
  RECT rect;
  GetClientRect(hWnd, &rect);
  SetTextColor(hdc, RGB(0xFF, 0x00, 0x00));
  SetBkMode(hdc, TRANSPARENT);
  rect.left = x;
  rect.top = y;
  DrawTextW(hdc, text.c_str(), -1, &rect, DT_SINGLELINE | DT_NOCLIP);
}
#endif

} // namespace Mana
