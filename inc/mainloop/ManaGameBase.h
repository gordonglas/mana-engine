#pragma once

#include "ManaGlobals.h"
#include "os/WindowBase.h"
#include "utils/CommandLine.h"

namespace Mana {
class ManaGameBase {
 public:
  ManaGameBase() {}
  virtual ~ManaGameBase() {}

  bool Run(int argc, char** argv, xstring title);

  xstring& GetLastError() { return error_; }

  CommandLine& GetCommandLine() { return commandLine_; }

  int GetReturnCode() { return nReturnCode_; }

 protected:
  int argc_;
  char** argv_;
  xstring error_;
  xstring title_;
  CommandLine commandLine_;
  WindowBase* pWindow_;
  int nReturnCode_;

  virtual bool OnInit();
  virtual bool OnGameLoop() = 0;
  virtual bool OnShutdown() = 0;
};
}  // namespace Mana
