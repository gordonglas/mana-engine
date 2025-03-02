#pragma once

#include "ManaGlobals.h"
#include "os/WindowBase.h"

namespace Mana {

class WindowWin : public WindowBase {
 public:
  WindowWin(HINSTANCE hInstance, int nCmdShow, WNDPROC wndProc)
      : hInstance_(hInstance),
        nCmdShow_(nCmdShow),
        hWnd_(nullptr),
        wndProc_(wndProc),
        previousWindowPlacement_({}) {}
  ~WindowWin() final = default;

  WindowWin(const WindowWin&) = delete;
  WindowWin& operator=(const WindowWin&) = delete;

  bool CreateMainWindow(CommandLine& commandLine, const xstring& title) final;

  // TODO: x-platform values for nCmdShow
  bool ShowWindow(int nCmdShow) final;

  bool ToggleFullscreenWindowed() final;

  HINSTANCE& GetHInstance() { return hInstance_; }

  HWND& GetHWnd() { return hWnd_; }

 private:
  HINSTANCE hInstance_;
  int nCmdShow_;  // the value passed from wWinMain (not currently used and
                  // might not be needed?)
  HWND hWnd_;
  WNDPROC wndProc_;

  WINDOWPLACEMENT previousWindowPlacement_;
};

}  // namespace Mana
