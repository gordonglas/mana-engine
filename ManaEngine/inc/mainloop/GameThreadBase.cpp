#include "pch.h"
#include "mainloop/GameThreadBase.h"

#include <cassert>
#include "os/WindowBase.h"

namespace Mana {

GameThreadBase::GameThreadBase(WindowBase& window)
    : window_(window), pThread_(nullptr) {
  threadData_.threadFunc = GameThreadFunction;
}

bool GameThreadBase::Run() {
  assert(!pThread_);

  threadData_.data = this;
  pThread_ = ThreadFactory::Create(&threadData_);
  if (!pThread_) {
    return false;
  }

  pThread_->Start();
  return true;
}

unsigned long GameThreadFunction(void* data) {
  GameThreadBase* game = (GameThreadBase*)data;

  if (game->OnInit()) {
    game->OnRunGameLoop();
  }

  //return game->OnShutdown();
  return 0;
}

}  // namespace Mana
