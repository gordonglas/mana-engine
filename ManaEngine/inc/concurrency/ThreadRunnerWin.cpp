#include "pch.h"
#include "concurrency/ThreadRunnerWin.h"

#include <cassert>
#include "os/WindowWin.h"

namespace Mana {

ThreadRunnerWin::ThreadRunnerWin() : pWindow_(nullptr) {}

void ThreadRunnerWin::RunOnMainThreadAsync(std::function<void()> func) {
  assert(pWindow_);
  queueAsync_.Push(func);
  ::PostMessageW(pWindow_->GetHWnd(), WM_RUN_ON_MAIN_THREAD_ASYNC, 0, 0);
}

}  // namespace Mana
