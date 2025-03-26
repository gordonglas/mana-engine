#pragma once

#include "ManaGlobals.h"
#include <atomic>
#include <functional>
#include "datastructures/SynchronizedQueue.h"

#define WM_RUN_ON_MAIN_THREAD       (WM_USER + 1)
#define WM_RUN_ON_MAIN_THREAD_ASYNC (WM_USER + 2)

namespace Mana {

class WindowWin;

class ThreadRunnerWin {
 public:
  ThreadRunnerWin();
  virtual ~ThreadRunnerWin();

  ThreadRunnerWin(const ThreadRunnerWin&) = delete;
  ThreadRunnerWin& operator=(const ThreadRunnerWin&) = delete;

  void SetWindow(WindowWin* window) { pWindow_ = window; }

  // Run |func| on main thread, synchronously.
  // Blocks until |func| has finished running on the main thread.
  void RunOnMainThread(std::function<void()> func);

  // Run |func| on main thread, asynchronously. Doesn't block.
  void RunOnMainThreadAsync(std::function<void()> func);

  SynchronizedQueue<std::function<void()>>& GetSyncQueue() {
    return queue_;
  }

  SynchronizedQueue<std::function<void()>>& GetAsyncQueue() {
    return queueAsync_;
  }

 private:
  WindowWin* pWindow_;
  SynchronizedQueue<std::function<void()>> queue_;
  SynchronizedQueue<std::function<void()>> queueAsync_;
  std::atomic<bool> isShuttingDown_;
};

}  // namespace Mana
