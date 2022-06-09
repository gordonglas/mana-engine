#include "pch.h"
#include "concurrency/IThread.h"
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
    if (m_hWait) {
      CloseHandle(m_hWait);
      m_hWait = nullptr;
    }
  }

  bool Init(ThreadFunc pThreadFunc) override;

  void Start() override {
    ScopedCriticalSection lock(m_lock);
    ResumeThread(m_hThread);
  }

  void Stop() override {
    m_bStopping = true;
    ScopedCriticalSection lock(m_lock);
    if (m_hWait) {
      SetEvent(m_hWait);
    }
    // TODO: m_queue.Clear();
  }

  bool IsStopping() override {
    return m_bStopping;
  }

  void Join() override {
    // thread is signaled when it exits
    WaitForSingleObject(m_hThread, INFINITE);
  }

  void EnqueueWorkItem(IWorkItem* pWorkItem) override {
    ScopedCriticalSection lock(m_lock);
    m_list.push_back(pWorkItem);
    m_queue.Push(pWorkItem);
    SetEvent(m_hWait);
  }

  bool IsAllItemsProcessed() override {
    ScopedCriticalSection lock(m_lock);

    for (const auto& item : m_list) {
      if (item->GetHandleIfDoneProcessing() == 0)
        return false;
    }

    return true;
  }

  void ClearProcessedItems() override {
    ScopedCriticalSection lock(m_lock);
    m_list.clear();
  }

 private:
  bool m_bInitialized = false;
  CriticalSection m_lock;
  HANDLE m_hThread = nullptr;
  // TODO: std::vector<shared_ptr<IWorkItem>>
  std::vector<IWorkItem*> m_list;
  SynchronizedQueue<IWorkItem*> m_queue;
  HANDLE m_hWait = nullptr;
  std::atomic<bool> m_bStopping = false;
  ThreadFunc m_pThreadFunc = nullptr;

  friend DWORD WINAPI ThreadFunction(LPVOID lpParam);
};

bool Thread::Init(ThreadFunc pThreadFunc) {
  ScopedCriticalSection lock(m_lock);

  if (m_bInitialized)
    return false;

  m_pThreadFunc = pThreadFunc;

  m_hThread = ::CreateThread(nullptr,  // default security attributes
                             0,        // use default stack size
                             ThreadFunction, this,
                             CREATE_SUSPENDED,  // user must call Thread::Start
                             nullptr);

  if (!m_hThread) {
    ManaLogLnError(Channel::Init, _X("CreateThread failed"));
    return false;
  }

  m_bInitialized = true;
  return true;
}

DWORD WINAPI ThreadFunction(LPVOID lpParam) {
  Thread* pThread = (Thread*)lpParam;

  ManaLogLnInfo(Channel::Init, _X("ThreadFunction"));

  if (pThread->m_pThreadFunc) {
    return pThread->m_pThreadFunc(pThread);
  } else {
    // TODO: does nullptr work for lpName when using multiple threads?
    pThread->m_hWait = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (pThread->m_hWait == nullptr) {
      ManaLogLnError(Channel::Init, _X("ThreadFunction: CreateEventW failed"));
      return 0;
    }

    while (!pThread->IsStopping()) {
      if (pThread->m_queue.Size() == 0) {
        WaitForSingleObject(pThread->m_hWait, INFINITE);

        if (pThread->IsStopping()) {
          break;
        }
      }

      while (pThread->m_queue.Size() > 0) {
        IWorkItem* pWorkItem = pThread->m_queue.Pop();
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
