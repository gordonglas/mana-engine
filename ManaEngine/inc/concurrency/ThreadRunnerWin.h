#pragma once

#include "ManaGlobals.h"
#include "concurrency/ThreadRunnerBase.h"

#define WM_RUN_ON_MAIN_THREAD       (WM_USER + 1)
#define WM_RUN_ON_MAIN_THREAD_ASYNC (WM_USER + 2)

namespace Mana {

class WindowWin;

class ThreadRunnerWin : public ThreadRunnerBase {
 public:
  ThreadRunnerWin();
  virtual ~ThreadRunnerWin();

  ThreadRunnerWin(const ThreadRunnerWin&) = delete;
  ThreadRunnerWin& operator=(const ThreadRunnerWin&) = delete;

  void SetWindow(WindowBase* window) override;

  void RunOnMainThread(std::function<void()> func) override;

  void RunOnMainThreadAsync(std::function<void()> func) override;

 private:
  WindowWin* pWindow_;
};

}  // namespace Mana
