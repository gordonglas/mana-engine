// cooperative multitasking process manager

#pragma once

#include <list>
#include "mainloop/ProcessBase.h"

namespace Mana {

class ProcessManager {
  typedef std::list<StrongProcessPtr> ProcessList;

 public:
  ProcessManager() = default;
  virtual ~ProcessManager();

  ProcessManager(const ProcessManager&) = delete;
  ProcessManager& operator=(const ProcessManager&) = delete;

  // interface
  unsigned int UpdateProcesses(unsigned long deltaMs);
  WeakProcessPtr AttachProcess(StrongProcessPtr pProcess);
  void AbortAllProcesses(bool immediate);

  // accessors
  unsigned int GetProcessCount() const { return (int)processList_.size(); }

 private:
  ProcessList processList_;

  void ClearAllProcesses();  // should only be called by destructor
};

}  // namespace Mana
