#pragma once

#include "ManaGlobals.h"
#include "concurrency/IThread.h"

namespace Mana {

class IThread;
class WindowBase;
class ThreadRunnerBase;

unsigned long GameThreadFunction(void* data);

class GameThreadBase {
 public:
  GameThreadBase(WindowBase& window, ThreadRunnerBase& threadRunner
                 /*, ConfigManager& configManager*/);
  virtual ~GameThreadBase() = default;

  GameThreadBase(const GameThreadBase&) = delete;
  GameThreadBase& operator=(const GameThreadBase&) = delete;

  bool Run();

  xstring& GetLastError() { return error_; }

  virtual bool OnShutdown() = 0;

 protected:
  // TODO: Not sure I like passing WindowBase into GameThread.
  //       If the GameThread makes changes to WindowBase,
  //       ManaGame will probably need to act on it.
  //       Find a different solution if/when needed.
  WindowBase& window_;

  // Used to run code on the main thread
  ThreadRunnerBase& threadRunner_;

  // The actual Thread used for the Game Thread and the data passed to it.
  IThread* pThread_;
  ThreadData threadData_;

  xstring error_;

  virtual bool OnInit() = 0;
  virtual bool OnRunGameLoop() = 0;

  virtual void PostQuitMessageWithPossibleError(
      const xstring& error = xstring()) = 0;

  friend unsigned long GameThreadFunction(void* data);
};

}  // namespace Mana
