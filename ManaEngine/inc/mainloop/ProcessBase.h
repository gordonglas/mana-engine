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

  ProcessBase(const ProcessBase&) = delete;
  ProcessBase& operator=(const ProcessBase&) = delete;

  // Functions for ending the process.
  inline void Succeed(void);
  inline void Fail(void);

  // pause
  inline void Pause(void);
  inline void UnPause(void);

  // accessors
  State GetState(void) const { return state_; }
  bool IsAlive(void) const {
    return (state_ == State::RUNNING || state_ == State::PAUSED);
  }
  bool IsDead(void) const {
    return (state_ == State::SUCCEEDED || state_ == State::FAILED ||
            state_ == State::ABORTED);
  }
  bool IsRemoved(void) const { return (state_ == State::REMOVED); }
  bool IsPaused(void) const { return state_ == State::PAUSED; }

  // child functions
  inline void AttachChild(StrongProcessPtr pChild);
  StrongProcessPtr RemoveChild(void);  // releases ownership of the child
  StrongProcessPtr PeekChild(void) {
    return pChild_;
  }  // doesn’t release ownership of child

 protected:
  // interface;
  // these functions should be overridden by the subclass as needed
  virtual void VOnInit(void) { state_ = State::RUNNING; }
  virtual void VOnUpdate(unsigned long deltaMs) = 0;
  virtual void VOnSuccess(void) {}
  virtual void VOnFail(void) {}
  virtual void VOnAbort(void) {}

 private:
  State state_;
  StrongProcessPtr pChild_;  // child process is optional

  void SetState(State newState) { state_ = newState; }

  friend class ProcessManager;
};

// inline function definitions --------------------------------------------

inline void ProcessBase::Succeed() {
  //GCC_ASSERT(state_ == RUNNING || state_ == PAUSED);
  state_ = State::SUCCEEDED;
}

inline void ProcessBase::Fail() {
  //GCC_ASSERT(state_ == RUNNING || state_ == PAUSED);
  state_ = State::FAILED;
}

inline void ProcessBase::AttachChild(StrongProcessPtr pChild) {
  if (pChild_)
    pChild_->AttachChild(pChild);
  else
    pChild_ = pChild;
}

inline void ProcessBase::Pause() {
  if (state_ == State::RUNNING)
    state_ = State::PAUSED;
  //else
  //  GCC_WARNING("Attempting to pause a process that isn't running");
}

inline void ProcessBase::UnPause() {
  if (state_ == State::PAUSED)
    state_ = State::RUNNING;
  //else
  //  GCC_WARNING("Attempting to unpause a process that isn't paused");
}

//inline StrongProcessPtr ProcessBase::GetTopLevelProcess()
//{
//  if (m_pParent)
//    return m_pParent->GetTopLevelProcess();
//  else
//    return this;
//}

}  // namespace Mana
