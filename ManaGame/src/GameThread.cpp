#include "GameThread.h"

#include <cassert>
#include "audio/AudioWin.h"
#include "concurrency/IThread.h"
#include "concurrency/ThreadRunnerWin.h"
#include "events/EventManager.h"
#include "os/WindowBase.h"
#include "os/WindowWin.h"

namespace Mana {

uint64_t g_fps;

GameThread::GameThread(WindowBase& window, ThreadRunnerWin& threadRunner)
    : GameThreadBase(window), threadRunner_(threadRunner) {}

bool GameThread::OnInit() {
  // TODO: Initialize COM in this thread.
  // TODO: Move most of ManaGame::OnInit() to here.
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
      // TODO: This is just for a dirty display of the FPS (using WM_PAINT/GDI)
      // until getting proper DirectX rendering working.
      threadRunner_.RunOnMainThreadAsync([&window]() {
        InvalidateRect(window.GetHWnd(), nullptr, TRUE);
      });

      // Example of running code on main thread synchronously.
      //if (error) {
      //  threadRunner_.RunOnMainThread([&window]() {
      //    MessageBoxW(window.GetHWnd(), L"Epic fail occurred", L"ERROR", MB_OK);
      //    PostQuitMessage(0);
      //  });
      //  return false;
      //}
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
