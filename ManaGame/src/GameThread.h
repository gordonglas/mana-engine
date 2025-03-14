#pragma once

#include "mainloop/GameThreadBase.h"

namespace Mana {

extern uint64_t g_fps;

class GameThread : public GameThreadBase {
 public:
  GameThread(WindowBase& window);
  virtual ~GameThread() = default;

 protected:
  bool OnInit() override;
  bool OnRunGameLoop() override;
  bool OnShutdown() override;
};

}  // namespace Mana
