#include "pch.h"
#include "mainloop/ProcessBase.h"

namespace Mana {

ProcessBase::ProcessBase() {
  m_state = State::UNINITIALIZED;
}

ProcessBase::~ProcessBase() {
  if (m_pChild)
    m_pChild->VOnAbort();
}

// Removes the child from this process.
// This releases ownership of the child to the caller
// and completely removes it from the process chain.
StrongProcessPtr ProcessBase::RemoveChild() {
  if (m_pChild) {
    // this keeps the child from getting destroyed when we clear it
    StrongProcessPtr pChild = m_pChild;
    m_pChild.reset();
    return pChild;
  }

  return StrongProcessPtr();
}

}  // namespace Mana
