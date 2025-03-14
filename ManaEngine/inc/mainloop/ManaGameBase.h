#pragma once

#include "ManaGlobals.h"
#include "os/WindowBase.h"
#include "utils/CommandLine.h"

namespace Mana {

class GameThreadBase;

class ManaGameBase {
 public:
  ManaGameBase();
  virtual ~ManaGameBase() = default;

  ManaGameBase(const ManaGameBase&) = delete;
  ManaGameBase& operator=(const ManaGameBase&) = delete;

  bool Run(int argc, char** argv, const xstring& title);

  xstring& GetLastError() { return error_; }

  CommandLine& GetCommandLine() { return commandLine_; }

  int GetReturnCode() { return nReturnCode_; }

  //virtual uint64_t GetFps() = 0;

  //virtual void OnGameLoopIteration() = 0;

 protected:
  int argc_;
  char** argv_;
  xstring error_;
  xstring title_;
  CommandLine commandLine_;
  WindowBase* pWindow_;
  GameThreadBase* gameThread_;
  int nReturnCode_;

  virtual bool OnInit();
  virtual bool OnStartGameThread() = 0;
  virtual bool OnRunMessageLoop() = 0;
  virtual bool OnShutdown() = 0;
};

}  // namespace Mana
