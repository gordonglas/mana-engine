#include "pch.h"
#include "concurrency/IThread.h"
#include "concurrency/NullWorkItem.h"
#include "utils/Lock.h"
#include "utils/Log.h"
#include "datastructures/SynchronizedQueue.h"
#include <atomic>
#include <vector>

namespace Mana {

DWORD WINAPI ThreadFunction(LPVOID lpParam);

class Thread : public IThread {
 public:
  ~Thread() override {
    if (hWait_) {
      CloseHandle(hWait_);
      hWait_ = nullptr;
    }
  }

  bool Init(ThreadFunc pThreadFunc) override;

  void Start() override {
    ScopedCriticalSection lock(lock_);
    ResumeThread(hThread_);
  }

  void Stop() override {
    bStopping_ = true;
    ScopedCriticalSection lock(lock_);
    if (hWait_) {
      SetEvent(hWait_);
    }
    // TODO: m_queue.Clear();
  }

  bool IsStopping() override {
    return bStopping_;
  }

  void Join() override {
    // thread is signaled when it exits
    WaitForSingleObject(hThread_, INFINITE);
  }

  void EnqueueWorkItem(IWorkItem* pWorkItem) override {
    ScopedCriticalSection lock(lock_);
    list_.push_back(pWorkItem);
    queue_.Push(pWorkItem);
    SetEvent(hWait_);
  }

  bool IsAllItemsProcessed() override {
    ScopedCriticalSection lock(lock_);

    for (const auto& item : list_) {
      if (item->GetHandleIfDoneProcessing() == 0)
        return false;
    }

    return true;
  }

  void ClearProcessedItems() override {
    ScopedCriticalSection lock(lock_);
    list_.clear();
  }

 private:
  bool bInitialized_ = false;
  CriticalSection lock_;
  HANDLE hThread_ = nullptr;
  // TODO: shared_ptr<IWorkItem>
  std::vector<IWorkItem*> list_;
  SynchronizedQueue<IWorkItem*> queue_;
  HANDLE hWait_ = nullptr;
  std::atomic<bool> bStopping_ = false;
  ThreadFunc pThreadFunc_ = nullptr;

  friend DWORD WINAPI ThreadFunction(LPVOID lpParam);
};

bool Thread::Init(ThreadFunc pThreadFunc) {
  ScopedCriticalSection lock(lock_);

  if (bInitialized_)
    return false;

  pThreadFunc_ = pThreadFunc;

  hThread_ = ::CreateThread(nullptr,  // default security attributes
                             0,       // use default stack size
                             ThreadFunction, this,
                             CREATE_SUSPENDED,  // user must call Thread::Start
                             nullptr);

  if (!hThread_) {
    ManaLogLnError(Channel::Init, _X("CreateThread failed"));
    return false;
  }

  bInitialized_ = true;
  return true;
}

DWORD WINAPI ThreadFunction(LPVOID lpParam) {
  Thread* pThread = (Thread*)lpParam;

  ManaLogLnInfo(Channel::Init, _X("ThreadFunction"));

  if (pThread->pThreadFunc_) {
    return pThread->pThreadFunc_(pThread);
  } else {
    // TODO: does nullptr work for lpName when using multiple threads?
    pThread->hWait_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (pThread->hWait_ == nullptr) {
      ManaLogLnError(Channel::Init, _X("ThreadFunction: CreateEventW failed"));
      return 0;
    }

    IWorkItem* pNullWorkItem = new NullWorkItem();

    while (!pThread->IsStopping()) {
      IWorkItem* pWorkItem = pThread->queue_.Pop(pNullWorkItem);
      // if no items in the queue (no work to do)..
      if (pWorkItem->GetType() == WorkItemType::NullWork) {
        // ...wait until items are put in the queue
        WaitForSingleObject(pThread->hWait_, INFINITE);

        if (pThread->IsStopping()) {
          break;
        }
      } else {
        pWorkItem->Process();
      }

      // keep processing work items until there are none
      while (1) {
        pWorkItem = pThread->queue_.Pop(pNullWorkItem);
        if (pWorkItem->GetType() == WorkItemType::NullWork) {
          break;
        }
        pWorkItem->Process();
      }
    }

    return 0;
  }
}

namespace ThreadFactory {

// TODO: do we really need this? leave it for now in case we want to add more
// properties/management of the threads.
class PrivateThreadFactory {
 public:
  static PrivateThreadFactory& Instance() {
    static PrivateThreadFactory instance;
    return instance;
  }

  IThread* CreateThread(ThreadFunc pThreadFunc);
};

IThread* PrivateThreadFactory::CreateThread(ThreadFunc pThreadFunc) {
  Thread* pThread = new Thread();
  if (!pThread) {
    return nullptr;
  }

  if (!pThread->Init(pThreadFunc)) {
    return nullptr;
  }

  return pThread;
}

// public factory function
IThread* Create(ThreadFunc pThreadFunc) {
  return PrivateThreadFactory::Instance().CreateThread(pThreadFunc);
}

}  // namespace ThreadFactory

}  // namespace Mana
