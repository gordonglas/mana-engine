#pragma once

#include "ManaGlobals.h"
#include "datastructures/SynchronizedQueue.h"
#include "input/InputBase.h"

namespace Mana {

// SynchronizedEventType tells us which fields are relevant being sent in the
// the SynchronizedEvent object, although we can usually tell this by fields
// that are set to specific values within it's InputAction fields anyway.
enum class SynchronizedEventType {
  Unknown,
  Input,
  InputDeviceChange,
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

  // queues up events going from main thread to the game-loop thread
  void EnqueueForGameLoop(SynchronizedEvent& event);

  SynchronizedQueue<SynchronizedEvent>& GetSyncQueue() { return syncQueue_; }
 private:
  SynchronizedQueue<SynchronizedEvent> syncQueue_;
};

}
