#include "pch.h"
#include "mainloop/GameThreadBase.h"

#include <cassert>
#include "concurrency/IThread.h"
#include "os/WindowBase.h"

namespace Mana {

GameThreadBase::GameThreadBase(WindowBase& window)
    : window_(window), pThread_(nullptr) {}

bool GameThreadBase::Run() {
  assert(!pThread_);

  pThread_ = ThreadFactory::Create(GameThreadFunction, this);
  if (!pThread_) {
    return false;
  }

  pThread_->Start();
  return true;
}

unsigned long GameThreadFunction(void* data) {
  GameThreadBase* game = (GameThreadBase*)data;

  if (game->OnInit()) {
    game->OnStartGameLoop();
  }

  return game->OnShutdown();
}

}  // namespace Mana
