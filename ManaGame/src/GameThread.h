#pragma once

#include "mainloop/GameThreadBase.h"

namespace Mana {

class GameThread : public GameThreadBase {
 public:
  GameThread(WindowBase& window);
  virtual ~GameThread() = default;

 protected:
  bool OnInit() override;
  bool OnStartGameLoop() override;
  bool OnShutdown() override;
};

}  // namespace Mana
