#pragma once

#include "ManaGlobals.h"
#include "concurrency/IThread.h"

namespace Mana {

class IThread;
class WindowBase;

unsigned long GameThreadFunction(void* data);

class GameThreadBase {
 public:
  GameThreadBase(WindowBase& window /*, ConfigManager& configManager*/);
  virtual ~GameThreadBase() = default;

  GameThreadBase(const GameThreadBase&) = delete;
  GameThreadBase& operator=(const GameThreadBase&) = delete;

  bool Run();

  virtual bool OnShutdown() = 0;

 protected:
  // TODO: Not sure I like passing WindowBase into GameThread.
  //       If the GameThread makes changes to WindowBase,
  //       ManaGame will probably need to act on it.
  //       Find a different solution if/when needed.
  WindowBase& window_;

  IThread* pThread_;

  ThreadData threadData_;

  virtual bool OnInit() = 0;
  virtual bool OnRunGameLoop() = 0;

  friend unsigned long GameThreadFunction(void* data);
};

}  // namespace Mana
