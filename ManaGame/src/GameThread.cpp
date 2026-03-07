#include "GameThread.h"

#include <cassert>
#include <vector>
#include "audio/AudioWin.h"
#include "audio/WorkItemLoadAudio.h"
#include "concurrency/IThread.h"
#include "concurrency/ThreadRunnerWin.h"
#include "events/EventManager.h"
#include "graphics/GraphicsDirectX11Win.h"
#include "os/WindowBase.h"
#include "os/WindowWin.h"

namespace Mana {

// TODO: Can probably make these vars GameThread members

uint64_t g_fps;

Mana::IThread* g_pLoadThread = nullptr;
Mana::ThreadData loadThreadData;

float g_masterVolume = 1.0f;
const float VolumeIncrement = 0.02f;
const float PanIncrement = 0.02f;
Mana::AudioFileHandle oggFile;
Mana::AudioFileHandle jumpSFX;

GameThread::GameThread(WindowWin& window, ThreadRunnerWin& threadRunner)
    : GameThreadBase(window, threadRunner) {}

bool GameThread::OnInit() {
  if (!com_.Init()) {
    error_ = _X("ComInitializer error");
    return false;
  }

  // init graphics engine
  g_pGraphicsEngine = new GraphicsDirectX11Win();
  g_pGraphicsEngine->Init();
  g_pGraphicsEngine->EnumerateAdaptersAndFullScreenModes();
  std::vector<GraphicsDeviceBase*> gpus;
  if (!g_pGraphicsEngine->GetSupportedGPUs(gpus)) {
    error_ = _X("GetSupportedGPUs failed");
    return false;
  }
  if (gpus.size() == 0) {
    error_ = g_pGraphicsEngine->GetNoSupportedGPUFoundMessage();
    return false;
  }

  // create device and device context
  if (!g_pGraphicsEngine->SelectGPU(gpus[0])) {
    error_ = _X("Failed to create gpu device");
    return false;
  }

  std::vector<Mana::MultisampleLevel> msaaLevels;
  if (!gpus[0]->GetSupportedMultisampleLevels(msaaLevels)) {
    error_ = _X("GetSupportedMultisampleLevels failed");
    return false;
  }

  // TODO: Is it safe to Release the IDXGIAdapter1 that we passed to
  // CreateDevice?
  //       Use ComPtr<T> to manage their lifetime.
  // for (GraphicsDeviceBase* gpu : gpus) {
  //  delete gpu;
  //}

  // init audio engine
  g_pAudioEngine = new AudioWin();
  g_pAudioEngine->Init();

  // instead of loading audio synchronously, we'll use a separate thread
  // while this main thread could render an animated "Loading" image.

  // the "load thread" will always exist throughout the life of the app,
  // but can be suspended when we don't need it,
  // so the OS scheduler won't uneccessarily context-switch to it.
  loadThreadData = {};
  g_pLoadThread = ThreadFactory::Create(&loadThreadData);
  g_pLoadThread->Start();

  // queue up the stuff that will be loaded in the load thread.
  // we call these "WorkItems"
  WorkItemLoadAudio* pLoadOgg = new WorkItemLoadAudio(
      g_pAudioEngine,
      _X("music/Kefka - NinjaGaiden - Evading the Enemy-loop.ogg"),
      AudioCategory::Music, AudioFormat::Ogg, 18060);

  // WorkItemLoadAudio* pLoadOgg = new WorkItemLoadAudio(
  //     g_pAudioEngine, _X("003 - Grandpa's Theme-loop.ogg"),
  //     AudioCategory::Music, AudioFormat::Ogg);

  WorkItemLoadAudio* pLoadJumpSFX =
      new WorkItemLoadAudio(g_pAudioEngine, _X("sound/jump001.ogg"),
                            AudioCategory::Sound, AudioFormat::Ogg, 0, 3);

  g_pLoadThread->EnqueueWorkItem(pLoadOgg);
  g_pLoadThread->EnqueueWorkItem(pLoadJumpSFX);

  // TODO: this should run in our game loop,
  //       since it has to show animation.

  // Wait for all work items (audio files) to finish loading.
  // We poll here instead of using a wait-function,
  // so we may render an animated "Loading" image.
  while (!g_pLoadThread->IsAllItemsProcessed()) {
    // Probably don't need to do a full-blown busy-wait.
    // Our Loading animation can still move.
    Sleep(100);

    // TODO: render Loading animation here
  }
  // TODO: Instead of clearing the processed items like this,
  //       maybe we can use shared_ptrs within the LoadThread instead?
  //       Although, we still need to get the handle like below. hmm
  g_pLoadThread->ClearProcessedItems();

  // cache the audio engine's sound handle, which we later use
  // to play/pause/stop/etc the sound
  oggFile = pLoadOgg->GetHandleIfDoneProcessing();
  jumpSFX = pLoadJumpSFX->GetHandleIfDoneProcessing();

  // the work items are not needed anymore
  delete pLoadOgg;
  pLoadOgg = nullptr;
  delete pLoadJumpSFX;
  pLoadJumpSFX = nullptr;

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

      if (syncEvents.size() > 0) {
        OutputDebugStringW((std::wstring(L"game-thread syncEvents: ") +
                            std::to_wstring(syncEvents.size()) + L"\n")
                               .c_str());

        for (SynchronizedEvent& event : syncEvents) {
          if (event.syncEventType == (U8)SynchronizedEventType::Input &&
              event.inputAction.deviceType == (U8)InputDeviceType::Keyboard) {
            switch (event.inputAction.virtualKey) {
              case VK_UP: {
                if (event.inputAction.flags & INPUTACTION_FLAG_RELEASE) {
                  OutputDebugStringW(L"game-thread VK_UP release\n");
                  // TODO: HERE!!! play our music file!!!
                  g_pAudioEngine->Play(oggFile, AudioBase::LOOP_INFINITE);
                } else {
                  OutputDebugStringW(L"game-thread VK_UP press\n");
                }
              } break;
            }
          }
        }
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

      WindowWin& window = static_cast<WindowWin&>(window_);
      // TODO: This is just for a dirty display of the FPS (using WM_PAINT/GDI)
      // until getting proper DirectX rendering working.
      threadRunner_.RunOnMainThreadAsync([&window]() {
        InvalidateRect(window.GetHWnd(), nullptr, TRUE);
      });
    }

    // TODO: OnRender(lag / (double)MICROSEC_PER_UPDATE);
  }

  return true;
}

void GameThread::PostQuitMessageWithPossibleError(const xstring& error) {
  WindowWin& window = static_cast<WindowWin&>(window_);
  // RunOnMainThread runs synchronously
  threadRunner_.RunOnMainThread([&window, error]() {
    if (!error.empty()) {
      // TODO: use SimpleMessageBox::Show instead, but pass the HWnd (or window obj)
      MessageBoxW(window.GetHWnd(), error.c_str(), L"ERROR",
                  MB_ICONERROR | MB_OK);
    }
    PostQuitMessage(0);
  });
}

bool GameThread::OnShutdown() {
  assert(!pThread_->IsStopping());

  pThread_->Stop();
  pThread_->Join();

  if (g_pLoadThread) {
    g_pLoadThread->Stop();
    g_pLoadThread->Join();
    delete g_pLoadThread;
    g_pLoadThread = nullptr;
  }

  if (g_pAudioEngine) {
    g_pAudioEngine->Uninit();
    delete g_pAudioEngine;
    g_pAudioEngine = nullptr;
  }

  if (g_pGraphicsEngine) {
    g_pGraphicsEngine->Uninit();
    delete g_pGraphicsEngine;
    g_pGraphicsEngine = nullptr;
  }

  return true;
}

}  // namespace Mana
