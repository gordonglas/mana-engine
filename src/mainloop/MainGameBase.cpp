#include "pch.h"
#include "mainloop/ManaGameBase.h"

namespace Mana {
bool ManaGameBase::OnInit() {
  if (!commandLine_.Parse(argc_, argv_)) {
    error_ = _X("CommandLine error");
    return false;
  }

  return true;
}

bool ManaGameBase::Run(int argc, char** argv, xstring title) {
  argc_ = argc;
  argv_ = argv;
  title_ = title;

  if (!OnInit())
    return false;

  OnGameLoop();

  return OnShutdown();
}
}  // namespace Mana
