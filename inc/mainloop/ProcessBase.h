// Base Process class for a cooperative multitasker

#pragma once

#include <memory>
#include "ManaGlobals.h"

namespace Mana {
class ProcessBase;
typedef std::shared_ptr<ProcessBase> StrongProcessPtr;
typedef std::weak_ptr<ProcessBase> WeakProcessPtr;

class ProcessBase {
 public:
  enum class State {
    // Processes that are neither dead nor alive
    UNINITIALIZED = 0,  // created but not running
    REMOVED,            // removed from the process list but not destroyed;
                        // this can happen when a process that is already
                        // running is parented to another process.
              // Living processes
    RUNNING,  // initialized and running
    PAUSED,   // initialized but paused

    // Dead processes
    SUCCEEDED,  // completed successfully
    FAILED,     // failed to complete
    ABORTED,    // aborted, may not have started
  };

  ProcessBase();
  virtual ~ProcessBase();

 protected:
  // interface;
  // these functions should be overridden by the subclass as needed
  virtual void VOnInit(void) { m_state = State::RUNNING; }
  virtual void VOnUpdate(unsigned long deltaMs) = 0;
  virtual void VOnSuccess(void) {}
  virtual void VOnFail(void) {}
  virtual void VOnAbort(void) {}

 public:
  // Functions for ending the process.
  inline void Succeed(void);
  inline void Fail(void);

  // pause
  inline void Pause(void);
  inline void UnPause(void);

  // accessors
  State GetState(void) const { return m_state; }
  bool IsAlive(void) const {
    return (m_state == State::RUNNING || m_state == State::PAUSED);
  }
  bool IsDead(void) const {
    return (m_state == State::SUCCEEDED || m_state == State::FAILED ||
            m_state == State::ABORTED);
  }
  bool IsRemoved(void) const { return (m_state == State::REMOVED); }
  bool IsPaused(void) const { return m_state == State::PAUSED; }

  // child functions
  inline void AttachChild(StrongProcessPtr pChild);
  StrongProcessPtr RemoveChild(void);  // releases ownership of the child
  StrongProcessPtr PeekChild(void) {
    return m_pChild;
  }  // doesn’t release
     // ownership of child

 private:
  State m_state;
  StrongProcessPtr m_pChild;  // child process is optional

  void SetState(State newState) { m_state = newState; }

  friend class ProcessManager;
};

// inline function definitions --------------------------------------------

inline void ProcessBase::Succeed() {
  // GCC_ASSERT(m_state == RUNNING || m_state == PAUSED);
  m_state = State::SUCCEEDED;
}

inline void ProcessBase::Fail() {
  // GCC_ASSERT(m_state == RUNNING || m_state == PAUSED);
  m_state = State::FAILED;
}

inline void ProcessBase::AttachChild(StrongProcessPtr pChild) {
  if (m_pChild)
    m_pChild->AttachChild(pChild);
  else
    m_pChild = pChild;
}

inline void ProcessBase::Pause() {
  if (m_state == State::RUNNING)
    m_state = State::PAUSED;
  // else
  //	GCC_WARNING("Attempting to pause a process that isn't running");
}

inline void ProcessBase::UnPause() {
  if (m_state == State::PAUSED)
    m_state = State::RUNNING;
  // else
  //	GCC_WARNING("Attempting to unpause a process that isn't paused");
}

/*
inline StrongProcessPtr ProcessBase::GetTopLevelProcess()
{
        if (m_pParent)
                return m_pParent->GetTopLevelProcess();
        else
                return this;
}
*/
}  // namespace Mana
