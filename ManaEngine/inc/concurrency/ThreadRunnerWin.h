#pragma once

#include "ManaGlobals.h"
#include <functional>
#include "datastructures/SynchronizedQueue.h"

#define WM_RUN_ON_MAIN_THREAD_ASYNC (WM_USER + 1)

namespace Mana {

class WindowWin;

class ThreadRunnerWin {
 public:
  ThreadRunnerWin();
  virtual ~ThreadRunnerWin() = default;

  ThreadRunnerWin(const ThreadRunnerWin&) = delete;
  ThreadRunnerWin& operator=(const ThreadRunnerWin&) = delete;

  void SetWindow(WindowWin* window) { pWindow_ = window; }

  void RunOnMainThreadAsync(std::function<void()> func);

  SynchronizedQueue<std::function<void()>>& GetAsyncQueue() {
    return queueAsync_;
  }

 private:
  WindowWin* pWindow_;
  SynchronizedQueue<std::function<void()>> queueAsync_;
};

}  // namespace Mana
