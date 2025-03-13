#pragma once

#include "concurrency/IWorkItem.h"

namespace Mana {

// A thread that can be used to process a queue of tasks,
// and when there's nothing to process, goes to sleep,
// and is woken up when an item is placed on the queue.
// Intended for background tasks, such as loading files.

class IThread;
typedef unsigned long (*ThreadFunc)(void* data);

// Thread interface.
// Either pass a pointer to your own ThreadFunc,
// or process work items via a queue with EnqueueWorkItem().
class IThread {
 public:
  IThread() {}
  virtual ~IThread() {}

  IThread(const IThread&) = delete;
  IThread& operator=(const IThread&) = delete;

  // TODO: maybe hide init somehow? make private with friend?
  virtual bool Init(ThreadFunc pThreadFunc, void* data) = 0;
  virtual void Start() = 0;
  // once stopped, you cannot call Start again
  virtual void Stop() = 0;
  virtual bool IsStopping() = 0;
  // wait until the thread exits
  // call Stop before calling Join
  virtual void Join() = 0;

  // queue an item to be processed
  virtual void EnqueueWorkItem(IWorkItem* pWorkItem) = 0;
  // poll to see if all items are done processing
  virtual bool IsAllItemsProcessed() = 0;
  // should probably be called after IsAllItemsProcessed returns true
  // TODO: This stinks. Find a safer way to clear the work items.
  virtual void ClearProcessedItems() = 0;
};

namespace ThreadFactory {
IThread* Create(ThreadFunc pThreadFunc = nullptr, void* data = nullptr);
};

}  // namespace Mana
