#include "GameThread.h"

#include "os/WindowBase.h"
#include "concurrency/IThread.h"

namespace Mana {

GameThread::GameThread(WindowBase& window)
    : GameThreadBase(window) {}

bool GameThread::OnInit() {
  return true;
}

bool GameThread::OnStartGameLoop() {
  return true;
}

bool GameThread::OnShutdown() {
  return true;
}

}  // namespace Mana
