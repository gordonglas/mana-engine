#pragma once

#include "mainloop/GameThreadBase.h"
#include "utils/ScopedComInitializer.h"

namespace Mana {

class WindowWin;
class ThreadRunnerWin;

extern uint64_t g_fps;

// Note: ctor, dtor, and OnShutdown are called on the main thread.
//       OnInit and OnRunGameLoop are called on the game thread.
class GameThread : public GameThreadBase {
 public:
  GameThread(WindowWin& window, ThreadRunnerWin& threadRunner);
  virtual ~GameThread() = default;

  bool OnShutdown() override;

 protected:
  bool OnInit() override;
  bool OnRunGameLoop() override;

  void PostQuitMessageWithPossibleError(
      const xstring& error = xstring()) override;

 private:
  ScopedComInitializer com_;
};

}  // namespace Mana
