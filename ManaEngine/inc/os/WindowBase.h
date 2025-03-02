#pragma once

#include "ManaGlobals.h"
#include "utils/CommandLine.h"

namespace Mana {

class WindowBase {
 public:
  WindowBase() : switchingWindowedMode_(false) {}
  virtual ~WindowBase() = default;

  WindowBase(const WindowBase&) = delete;
  WindowBase& operator=(const WindowBase&) = delete;

  xstring& GetLastError() { return error_; }

  virtual bool CreateMainWindow(CommandLine& commandLine,
                                const xstring& title) = 0;

  virtual bool ShowWindow(int nCmdShow) = 0;

  virtual bool ToggleFullscreenWindowed() = 0;

 protected:
  xstring error_;
  bool switchingWindowedMode_;
};

}  // namespace Mana
