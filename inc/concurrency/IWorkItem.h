#pragma once

#include "target/TargetOS.h"

namespace Mana {

enum class WorkItemType { LoadAudio };

class IWorkItem {
 public:
  IWorkItem() {}
  virtual ~IWorkItem() {}

  virtual WorkItemType GetType() = 0;
  virtual void Process() = 0;
  // returns 0 if not done processing yet
  virtual size_t GetHandleIfDoneProcessing() = 0;
};

}  // namespace Mana
