#pragma once

#include "target/TargetOS.h"

namespace Mana {

enum class WorkItemType { LoadAudio };

class IWorkItem {
 public:
  IWorkItem() = default;
  virtual ~IWorkItem() = default;

  IWorkItem(const IWorkItem&) = delete;
  IWorkItem& operator=(const IWorkItem&) = delete;

  virtual WorkItemType GetType() = 0;
  virtual void Process() = 0;
  // returns 0 if not done processing yet
  virtual size_t GetHandleIfDoneProcessing() = 0;
};

}  // namespace Mana
