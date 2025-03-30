#pragma once

#include "ManaGlobals.h"
#include <atomic>
#include <functional>
#include "datastructures/SynchronizedQueue.h"

namespace Mana {

class ThreadRunnerBase {
 public:
  ThreadRunnerBase() = default;
  virtual ~ThreadRunnerBase() = default;

  ThreadRunnerBase(const ThreadRunnerBase&) = delete;
  ThreadRunnerBase& operator=(const ThreadRunnerBase&) = delete;

  virtual void SetWindow(WindowBase* window) = 0;

  // Run |func| on main thread, synchronously.
  // Blocks until |func| has finished running on the main thread.
  virtual void RunOnMainThread(std::function<void()> func) = 0;

  // Run |func| on main thread, asynchronously. Doesn't block.
  virtual void RunOnMainThreadAsync(std::function<void()> funct) = 0;

  SynchronizedQueue<std::function<void()>>& GetSyncQueue() { return queue_; }

  SynchronizedQueue<std::function<void()>>& GetAsyncQueue() {
    return queueAsync_;
  }

 protected:
  SynchronizedQueue<std::function<void()>> queue_;
  SynchronizedQueue<std::function<void()>> queueAsync_;
  std::atomic<bool> isShuttingDown_;
};

}  // namespace Mana
