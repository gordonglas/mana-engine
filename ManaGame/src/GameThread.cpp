#include "GameThread.h"

#include <cassert>
#include "audio/AudioWin.h"
#include "concurrency/IThread.h"
#include "events/EventManager.h"
#include "os/WindowBase.h"
#include "os/WindowWin.h"

namespace Mana {

uint64_t g_fps;

GameThread::GameThread(WindowBase& window)
    : GameThreadBase(window) {}

bool GameThread::OnInit() {

  return true;
}

bool GameThread::OnRunGameLoop() {
  // Using a fixed timestep loop per Game Loop Pattern.
  // http://gameprogrammingpatterns.com/game-loop.html
  // The accumulator is in microseconds as a 64 bit unsigned int,
  // instead of double, to avoid floating-point rounding issues.

  g_clock.Reset();

  uint64_t current, elapsed;
  uint64_t previous = g_clock.GetMicroseconds();
  uint64_t lag = 0;  // aka, the accumulator

  // 60 frames per second =
  // 60 frames per 1000000 microseconds = 1000000 / 60 = ~16,666.
  // We actually want it smaller than 60 FPS
  uint64_t MICROSEC_PER_UPDATE = 16000;

  // TODO: Figure out what a good number is for this on
  //       our slowest supported machine. No idea yet.
  int MAX_UPDATES = 10;
  int max_updates;

  int numFrames = 0;
  uint64_t lastFPSCalculation = g_clock.GetMicroseconds();

  std::vector<SynchronizedEvent> syncEvents;

  while (!pThread_->IsStopping()) {
    current = g_clock.GetMicroseconds();
    elapsed = current - previous;
    if (elapsed < 0)
      elapsed = 0;
    previous = current;
    lag += elapsed;

    // get raw input events from the main thread
    if (!g_pEventMan->GetSyncQueue().Empty_NoLock()) {
      g_pEventMan->GetSyncQueue().PopAll(syncEvents);

      if (syncEvents.size() > 1) {
        OutputDebugStringW((std::wstring(L"game-loop syncEvents: ") +
                            std::to_wstring(syncEvents.size()) + L"\n")
                               .c_str());
      }
    }

    // TODO: OnProcessInput();

    max_updates = MAX_UPDATES;
    while (lag >= MICROSEC_PER_UPDATE && max_updates > 0) {
      // TODO: OnUpdate();
      g_pAudioEngine->Update();

      lag -= MICROSEC_PER_UPDATE;
      --max_updates;
    }

    ++numFrames;
    // if 1 second elapsed, recalculate FPS
    if (current - lastFPSCalculation >= 1000000) {
      g_fps = numFrames;
      numFrames = 0;
      lastFPSCalculation += 1000000;

      WindowWin& window = dynamic_cast<WindowWin&>(window_);
      // TODO: Technically InvalidateRect should be called on the main thread,
      //       but this is just for some dirty display of the FPS until
      //       we get proper DirectX rendering working.
      InvalidateRect(window.GetHWnd(), nullptr, TRUE);
    }

    // TODO: OnRender(lag / (double)MICROSEC_PER_UPDATE);
  }

  return true;
}

bool GameThread::OnShutdown() {
  assert(!pThread_->IsStopping());

  pThread_->Stop();
  pThread_->Join();
  return true;
}

}  // namespace Mana
