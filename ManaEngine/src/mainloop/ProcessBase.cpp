#include "pch.h"
#include "mainloop/ProcessBase.h"

namespace Mana {

ProcessBase::ProcessBase() : state_(State::UNINITIALIZED) {}

ProcessBase::~ProcessBase() {
  if (pChild_)
    pChild_->VOnAbort();
}

// Removes the child from this process.
// This releases ownership of the child to the caller
// and completely removes it from the process chain.
StrongProcessPtr ProcessBase::RemoveChild() {
  if (pChild_) {
    // this keeps the child from getting destroyed when we clear it
    StrongProcessPtr pChild = pChild_;
    pChild_.reset();
    return pChild;
  }

  return StrongProcessPtr();
}

}  // namespace Mana
