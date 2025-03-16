#pragma once

#include "mainloop/GameThreadBase.h"

namespace Mana {

class ThreadRunnerWin;

extern uint64_t g_fps;

class GameThread : public GameThreadBase {
 public:
  GameThread(WindowBase& window, ThreadRunnerWin& threadRunner);
  virtual ~GameThread() = default;

 protected:
  bool OnInit() override;
  bool OnRunGameLoop() override;
  bool OnShutdown() override;

 private:
  ThreadRunnerWin& threadRunner_;
};

}  // namespace Mana
