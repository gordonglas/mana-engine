#include "pch.h"
#include "concurrency/IThread.h"
#include "concurrency/Mutex.h"
#include "utils/Log.h"
#include "datastructures/SynchronizedQueue.h"
#include <atomic>
#include <cassert>
#include <functional>
#include <optional>
#include <vector>

namespace Mana {

DWORD WINAPI ThreadFunction(LPVOID lpParam);

// A cross-thread-safe Thread implementation.
class Thread : public IThread {
 public:
  ~Thread() override {
    if (hWait_) {
      CloseHandle(hWait_);
      hWait_ = nullptr;
    }
  }

  bool Init(ThreadData* threadData) override;

  void Start() override {
    ScopedMutex lock(lock_);
    ResumeThread(hThread_);
  }

  void Stop() override {
    // TODO: bStopping_.store(true, std::memory_order_release);
    bStopping_ = true;
    ScopedMutex lock(lock_);
    if (hWait_) {
      SetEvent(hWait_);
    }
    // TODO: m_queue.Clear();
  }

  bool IsStopping() override {
    // TODO: .load(std::memory_order_acquire)
    return bStopping_ == true;
  }

  void Join() override {
    // thread is signaled when it exits
    WaitForSingleObject(hThread_, INFINITE);
  }

  void EnqueueWorkItem(IWorkItem* pWorkItem) override {
    ScopedMutex lock(lock_);
    list_.push_back(pWorkItem);
    queue_.Push(pWorkItem);
    SetEvent(hWait_);
  }

  bool IsAllItemsProcessed() override {
    ScopedMutex lock(lock_);

    for (const auto& item : list_) {
      if (item->GetHandleIfDoneProcessing() == 0)
        return false;
    }

    return true;
  }

  void ClearProcessedItems() override {
    ScopedMutex lock(lock_);
    list_.clear();
  }

 private:
  bool bInitialized_ = false;
  Mutex lock_;
  HANDLE hThread_ = nullptr;
  // TODO: shared_ptr<IWorkItem>
  std::vector<IWorkItem*> list_;
  SynchronizedQueue<IWorkItem*> queue_;
  HANDLE hWait_ = nullptr;
  std::atomic<bool> bStopping_ = false;
  ThreadFunc pThreadFunc_ = nullptr;

  friend DWORD WINAPI ThreadFunction(LPVOID lpParam);
};

bool Thread::Init(ThreadData* threadData) {
  ScopedMutex lock(lock_);

  if (bInitialized_)
    return false;

  pThreadFunc_ = threadData->threadFunc;
  if (!threadData->data) {
    threadData->data = this;
  }

  hThread_ = ::CreateThread(nullptr,  // default security attributes
                            0,        // use default stack size
                            ThreadFunction, threadData,
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
  ThreadData* threadData = (ThreadData*)lpParam;

  ManaLogLnInfo(Channel::Init, _X("ThreadFunction"));

  if (threadData->threadFunc) {
    return threadData->threadFunc(threadData->data);
  } else {
    assert(threadData->data);
    Thread* pThread = (Thread*)threadData->data;
    // TODO: does nullptr work for lpName when using multiple threads?
    pThread->hWait_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (pThread->hWait_ == nullptr) {
      ManaLogLnError(Channel::Init, _X("ThreadFunction: CreateEventW failed"));
      return 0;
    }

    while (!pThread->IsStopping()) {
      std::optional<IWorkItem*> workItem =
          pThread->queue_.Pop();

      if (!workItem.has_value()) {
        WaitForSingleObject(pThread->hWait_, INFINITE);

        if (pThread->IsStopping()) {
          break;
        }
      } else {
        workItem.value()->Process();
      }

      while (1) {
        workItem = pThread->queue_.Pop();
        if (!workItem.has_value()) {
          break;
        }
        workItem.value()->Process();
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

  IThread* CreateThread(ThreadData* threadData);
};

IThread* PrivateThreadFactory::CreateThread(ThreadData* threadData) {
  Thread* pThread = new Thread();
  if (!pThread) {
    return nullptr;
  }

  if (!pThread->Init(threadData)) {
    return nullptr;
  }

  return pThread;
}

// public factory function
// TODO: passing ThreadData is ok, but it's fields need to be made clearer
//       to the user.
IThread* Create(ThreadData* threadData) {
  assert(threadData);
  return PrivateThreadFactory::Instance().CreateThread(threadData);
}

}  // namespace ThreadFactory

}  // namespace Mana
