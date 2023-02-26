// cooperative multitasking process manager

#pragma once

#include <list>
#include "mainloop/ProcessBase.h"

namespace Mana {

class ProcessManager {
  typedef std::list<StrongProcessPtr> ProcessList;

 public:
  ~ProcessManager();

  // interface
  unsigned int UpdateProcesses(unsigned long deltaMs);
  WeakProcessPtr AttachProcess(StrongProcessPtr pProcess);
  void AbortAllProcesses(bool immediate);

  // accessors
  unsigned int GetProcessCount() const { return (int)m_processList.size(); }

 private:
  ProcessList m_processList;

  void ClearAllProcesses();  // should only be called by destructor
};

}  // namespace Mana
