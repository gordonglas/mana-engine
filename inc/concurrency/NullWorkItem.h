#pragma once

#include "concurrency/IWorkItem.h"

namespace Mana {

// This class makes working with the SynchronizedThread class easier.
// It allows it's Pop operation to be fully thread-safe.
class NullWorkItem : public IWorkItem {
 public:
  NullWorkItem() {}
  ~NullWorkItem() override {}

  WorkItemType GetType() override { return WorkItemType::NullWork; }
  void Process() override {}
  size_t GetHandleIfDoneProcessing() override { return 1; }
};

}  // namespace Mana
