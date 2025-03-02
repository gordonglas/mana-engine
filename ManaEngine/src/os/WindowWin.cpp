#include "pch.h"

#include "os/WindowWin.h"

namespace Mana {

constexpr size_t MAX_LEN_TITLE = 100;
std::wstring windowClassName;

bool WindowWin::CreateMainWindow(CommandLine& commandLine,
                                 const xstring& title) {
  UNREFERENCED_PARAMETER(commandLine);

  if (title.size() > MAX_LEN_TITLE) {
    error_ = _X("title cannot be longer than MAX_LEN_TITLE");
    return false;
  }

  previousWindowPlacement_ = {sizeof(WINDOWPLACEMENT)};
  switchingWindowedMode_ = false;

  WNDCLASSEXW wcex;
  wcex.cbSize = sizeof(WNDCLASSEX);

  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = wndProc_;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hInstance = hInstance_;
  wcex.hIcon = ::LoadIconW(
      nullptr,
      IDI_APPLICATION);  // LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MANAGAME));
  wcex.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
  wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wcex.lpszMenuName = nullptr;  // MAKEINTRESOURCEW(IDC_MANAGAME);
  // window classes are process specific,
  // so their name only needs to be unique within the
  // same process.
  windowClassName = L"ManaEngineGameClass";
  wcex.lpszClassName = windowClassName.c_str();
  wcex.hIconSm =
      nullptr;  // LoadIconW(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

  if (::RegisterClassExW(&wcex) == 0) {
    error_ = _X("::RegisterClassExW failed");
    return false;
  }

  // TODO: size params

  /*
  HWND hWnd = ::CreateWindowExW(
    // TODO: is this param correct?
    //WS_EX_OVERLAPPEDWINDOW, //0,
    0,
    windowClassName.c_str(),
    title.c_str(),
    WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
    nullptr, nullptr, hInstance_, nullptr);
  */

  HWND hWnd =
      CreateWindowW(windowClassName.c_str(), title.c_str(), WS_OVERLAPPEDWINDOW,
                    CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr,
                    hInstance_, nullptr);

  if (!hWnd) {
    return false;
  }

  hWnd_ = hWnd;

  return true;
}

bool WindowWin::ShowWindow(int nCmdShow) {
  ::ShowWindow(hWnd_, nCmdShow);

  if (!::UpdateWindow(hWnd_)) {
    error_ = L"::UpdateWindow failed";
    return false;
  }

  return true;
}

bool WindowWin::ToggleFullscreenWindowed() {
  if (switchingWindowedMode_) {
    return false;
  }
  switchingWindowedMode_ = true;

  HWND hwnd = GetHWnd();
  DWORD dwStyle = GetWindowLongW(hwnd, GWL_STYLE);
  if (dwStyle & WS_OVERLAPPEDWINDOW) {
    MONITORINFO mi = {sizeof(mi)};
    if (GetWindowPlacement(hwnd, &previousWindowPlacement_) &&
        GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY),
                        &mi)) {
      SetWindowLongW(hwnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
      SetWindowPos(hwnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top,
                   mi.rcMonitor.right - mi.rcMonitor.left,
                   mi.rcMonitor.bottom - mi.rcMonitor.top,
                   SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
  } else {
    SetWindowLongW(hwnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
    SetWindowPlacement(hwnd, &previousWindowPlacement_);
    SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER |
                     SWP_FRAMECHANGED);
  }

  switchingWindowedMode_ = false;
  return true;
}

}  // namespace Mana
