// cooperative multitasking process manager

#pragma once

#include "mainloop/ProcessBase.h"
#include <list>

namespace Mana
{
	class ProcessManager
	{
		typedef std::list<StrongProcessPtr> ProcessList;
		
	public:
		~ProcessManager();

		// interface
		unsigned int UpdateProcesses(unsigned long deltaMs);
		WeakProcessPtr AttachProcess(StrongProcessPtr pProcess);
		void AbortAllProcesses(bool immediate);

		// accessors
		unsigned int GetProcessCount() const { return m_processList.size(); }

	private:
		ProcessList m_processList;

		void ClearAllProcesses(); // should only be called by destructor
	};
}
