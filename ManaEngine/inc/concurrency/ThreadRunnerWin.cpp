#include "pch.h"
#include "concurrency/ThreadRunnerWin.h"

#include <cassert>
#include <condition_variable>
#include <mutex>
#include "os/WindowWin.h"

namespace Mana {

ThreadRunnerWin::ThreadRunnerWin() : pWindow_(nullptr) {}

void ThreadRunnerWin::RunOnMainThread(std::function<void()> func) {
  assert(pWindow_);

  std::mutex mutex;
  std::condition_variable condition;
  bool functionCompleted = false;

  queue_.Push([&]() {
    func();

    {
      std::lock_guard<std::mutex> lock(mutex);
      functionCompleted = true;
    }

    condition.notify_one();
  });

  ::PostMessageW(pWindow_->GetHWnd(), WM_RUN_ON_MAIN_THREAD, 0, 0);

  {
    std::unique_lock<std::mutex> lock(mutex);
    condition.wait(lock, [&]() { return functionCompleted; });
  }
}

void ThreadRunnerWin::RunOnMainThreadAsync(std::function<void()> func) {
  assert(pWindow_);
  queueAsync_.Push(func);
  ::PostMessageW(pWindow_->GetHWnd(), WM_RUN_ON_MAIN_THREAD_ASYNC, 0, 0);
}

}  // namespace Mana
