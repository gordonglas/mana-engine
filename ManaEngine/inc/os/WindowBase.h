#pragma once

#include "ManaGlobals.h"
#include "utils/CommandLine.h"

namespace Mana {

class WindowBase {
 public:
  WindowBase() {}
  virtual ~WindowBase() {}

  xstring& GetLastError() { return error_; }

  virtual bool CreateMainWindow(CommandLine& commandLine, xstring& title) = 0;

  virtual bool ShowWindow(int nCmdShow) = 0;

  virtual bool ToggleFullscreenWindowed() = 0;

 protected:
  xstring error_;
  bool switchingWindowedMode_;
};

}  // namespace Mana
