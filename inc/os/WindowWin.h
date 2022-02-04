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
        wndProc_(wndProc) {}
  ~WindowWin() final {}

  bool CreateMainWindow(CommandLine& commandLine, xstring& title) final;

  bool ShowWindow() final;

  HINSTANCE& GetHInstance() { return hInstance_; }

  HWND& GetHWnd() { return hWnd_; }

 private:
  HINSTANCE hInstance_;
  int nCmdShow_;
  HWND hWnd_;
  WNDPROC wndProc_;
};
}  // namespace Mana
