#pragma once

#include "concurrency/IWorkItem.h"

namespace Mana {
class IThread;
typedef unsigned long (*ThreadFunc)(IThread* pThread);

// Thread interface.
// Either pass a pointer to your own ThreadFunc,
// or process work items via a queue with EnqueueWorkItem().
class IThread {
 public:
  IThread() {}
  virtual ~IThread() {}

  // TODO: maybe hide init somehow? make private with friend?
  virtual bool Init(ThreadFunc pThreadFunc) = 0;
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
  virtual void ClearProcessedItems() = 0;
};

namespace ThreadFactory {
IThread* Create(ThreadFunc pThreadFunc = nullptr);
};
}  // namespace Mana
