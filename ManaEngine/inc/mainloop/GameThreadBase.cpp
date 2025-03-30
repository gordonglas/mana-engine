#include "pch.h"
#include "mainloop/GameThreadBase.h"

#include <cassert>
#include "os/WindowBase.h"
#include "concurrency/ThreadRunnerBase.h"

namespace Mana {

GameThreadBase::GameThreadBase(WindowBase& window,
                               ThreadRunnerBase& threadRunner)
    : window_(window), threadRunner_(threadRunner), pThread_(nullptr) {
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

  if (!game->OnInit()) {
    assert(!game->GetLastError().empty());
    game->PostQuitMessageWithPossibleError(game->GetLastError());
    return 1;
  }

  // TODO: Need to know if game thread was told to exit by the main thread or not.
  //       If not, we need to tell the main thread to exit (without error).
  if (!game->OnRunGameLoop()) {
    assert(!game->GetLastError().empty());
    game->PostQuitMessageWithPossibleError(game->GetLastError());
    return 1;
  }

  // If pThread_->IsStopping(), we know the main thread told the game thread
  // to quit, so if we're NOT IsStopping, we need to tell the main thread to
  // quit. This handles the case when the user exits the game through a
  // game menu, for example (a way to exit from the game itself, not the Window.)
  if (!game->pThread_->IsStopping()) {
    game->PostQuitMessageWithPossibleError();
  }

  // OnShutdown is called from main thread,
  // so no need to call it here.
  return 0;
}

}  // namespace Mana
