#include "pch.h"
#include "mainloop/GameThreadBase.h"
#include "mainloop/ManaGameBase.h"

namespace Mana {

ManaGameBase::ManaGameBase()
    : argc_(0),
      argv_(nullptr),
      pWindow_(nullptr),
      gameThread_(nullptr),
      nReturnCode_(0) {}

bool ManaGameBase::OnInit() {
  if (!commandLine_.Parse(argc_, argv_)) {
    error_ = _X("CommandLine error");
    return false;
  }

  return true;
}

bool ManaGameBase::Run(int argc, char** argv, const xstring& title) {
  argc_ = argc;
  argv_ = argv;
  title_ = title;

  if (OnInit() && OnStartGameThread()) {
    OnRunMessageLoop();
  }

  return OnShutdown();
}

}  // namespace Mana
