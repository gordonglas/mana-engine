#pragma once

#include "ManaGlobals.h"
#include "datastructures/SynchronizedQueue.h"
#include "input/InputBase.h"

namespace Mana {

enum class SynchronizedEventType {
  Unknown,
  Input,
};

// events that are sent across the thread boundary
struct SynchronizedEvent {
  InputAction inputAction;
  U8 syncEventType;  // SynchronizedEventType
};

// TODO: create the g_pEventMan var somewhere. Either global, or under g_pGame?

class EventManager;
extern EventManager* g_pEventMan;

class EventManager {
 public:
  bool Init() { return true; }
  void Uninit() {}
  void EnqueueForGameLoop(SynchronizedEvent& event);

  SynchronizedQueue<SynchronizedEvent>& GetSyncQueue() { return syncQueue_; }
 private:
  SynchronizedQueue<SynchronizedEvent> syncQueue_;
};

}
