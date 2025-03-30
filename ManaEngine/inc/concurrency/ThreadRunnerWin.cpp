#include "pch.h"
#include "concurrency/ThreadRunnerWin.h"

#include <cassert>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include "os/WindowWin.h"

namespace Mana {

ThreadRunnerWin::ThreadRunnerWin() : pWindow_(nullptr) {}

ThreadRunnerWin::~ThreadRunnerWin() {
  isShuttingDown_ = true;
}

void ThreadRunnerWin::SetWindow(WindowBase* window) {
  pWindow_ = static_cast<WindowWin*>(window);
}

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
    // Loop while calling wait_for (with timeout), so we can check the
    // isShuttingDown_ bool, to handle the rare case when the main thread exits
    // before processing WM_RUN_ON_MAIN_THREAD, to prevent this thread from
    // never exiting in that case.
    while (!functionCompleted && !isShuttingDown_) {
      condition.wait_for(lock, std::chrono::milliseconds(500),
                         [&]() { return functionCompleted; });
    }
  }
}

void ThreadRunnerWin::RunOnMainThreadAsync(std::function<void()> func) {
  assert(pWindow_);
  queueAsync_.Push(func);
  ::PostMessageW(pWindow_->GetHWnd(), WM_RUN_ON_MAIN_THREAD_ASYNC, 0, 0);
}

}  // namespace Mana
