#include "pch.h"
#include "events/EventManager.h"

namespace Mana {

EventManager* g_pEventMan;

void EventManager::EnqueueForGameLoop(SynchronizedEvent& event) {
  syncQueue_.Push(event);
}

} // namespace Mana
