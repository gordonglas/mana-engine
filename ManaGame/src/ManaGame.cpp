#include "Globals.h"
#include "target/TargetOS.h"
#include <atomic>
#include <shellapi.h>
#include <string.h>
#include "audio/WorkItemLoadAudio.h"
#include "concurrency/IThread.h"
#include "concurrency/IWorkItem.h"
#include "concurrency/NamedMutex.h"
#include "config/ConfigManager.h"
#include "debugging/DebugWin.h"
#include "events/EventManager.h"
#include "input/InputWin.h"
#include "mainloop/ManaGameBase.h"
#include "os/WindowWin.h"
#include "ui/SimpleMessageBox.h"
#include "utils/Com.h"
#include "utils/CommandLine.h"
#include "utils/Strings.h"

#include "audio/AudioWin.h"
float g_masterVolume = 1.0f;
const float VolumeIncrement = 0.02f;
const float PanIncrement = 0.02f;
Mana::AudioFileHandle oggFile;
Mana::AudioFileHandle jumpSFX;

Mana::IThread* g_pLoadThread = nullptr;

#define MAX_LOADSTRING 100

WCHAR szTitle[MAX_LOADSTRING];        // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];  // the main window class name

Mana::Timer g_clock;
Mana::ManaGameBase* g_pGame;

// Forward declarations
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

namespace Mana {

uint64_t g_fps;

DWORD WINAPI GameLoopThreadFunction(LPVOID lpParam);

class GameLoopThread {
 public:
  bool Init(HWND hwnd);
  void Start() { ResumeThread(hThread_); }
  void Stop() {
    bStopping_.store(true, std::memory_order_release);
  }
  void Join() {
    // thread is signaled when it exits
    WaitForSingleObject(hThread_, INFINITE);
  }

  HWND hwnd_ = nullptr;

 private:
  HANDLE hThread_ = nullptr;
  std::atomic<bool> bStopping_ = false;

 friend DWORD WINAPI GameLoopThreadFunction(LPVOID lpParam);
};

bool GameLoopThread::Init(HWND hwnd) {
  hwnd_ = hwnd;
  hThread_ = ::CreateThread(nullptr,  // default security attributes
                            0,        // use default stack size
                            GameLoopThreadFunction, this,
                            CREATE_SUSPENDED,  // we must call Start()
                            nullptr);
  if (!hThread_) {
    ManaLogLnError(Channel::Init, _X("GameLoopThread CreateThread failed"));
    return false;
  }
  return true;
}

DWORD WINAPI GameLoopThreadFunction(LPVOID lpParam) {
  GameLoopThread* pThread = (GameLoopThread*)lpParam;

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

  while (!pThread->bStopping_.load(std::memory_order_acquire)) {
    current = g_clock.GetMicroseconds();
    elapsed = current - previous;
    if (elapsed < 0)
      elapsed = 0;
    previous = current;
    lag += elapsed;

    // get raw input events from the main thread
    if (!g_pEventMan->GetSyncQueue().Empty_NoLock()) {
      std::vector<SynchronizedEvent> syncEvents =
          g_pEventMan->GetSyncQueue().PopAll();

      if (syncEvents.size() > 0) {
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
      InvalidateRect(pThread->hwnd_, nullptr, TRUE);
    }

    // TODO: OnRender(lag / (double)MICROSEC_PER_UPDATE);
  }

  return 0;
}

GameLoopThread gameThread;

class ManaGame : public ManaGameBase {
 public:
  ManaGame(HINSTANCE hInstance, int nCmdShow)
      : hInstance_(hInstance), nCmdShow_(nCmdShow) {}
  ~ManaGame() final {}

  //uint64_t GetFps() final { return g_fps; }

  WindowWin* GetWindow() { return (WindowWin*)pWindow_; }

 protected:
  bool OnInit() final;
  bool OnStartGameLoop() final;
  bool OnShutdown() final;

 private:
  HINSTANCE hInstance_;
  int nCmdShow_;
};

bool ManaGame::OnInit() {
  ManaGameBase::OnInit();

  // TODO: init ConfigManager and load game configs

  pWindow_ = new WindowWin(hInstance_, nCmdShow_, WndProc);
  if (!pWindow_->CreateMainWindow(commandLine_, title_)) {
    error_ = _X("CreateMainWindow error");
    return false;
  }

  if (!ComInitilizer::Init()) {
    return false;
  }

  // init event manager
  g_pEventMan = new EventManager();
  g_pEventMan->Init();

  // init input engine
  g_pInputEngine = new InputWin(GetWindow()->GetHWnd());
  g_pInputEngine->Init();

  // init audio engine
  g_pAudioEngine = new AudioWin();
  g_pAudioEngine->Init();

  // instead of loading audio synchronously, we'll use a separate thread
  // while this main thread could render an animated "Loading" image.

  // the "load thread" will always exist throughout the life of the app,
  // but can be suspended when we don't need it,
  // so the OS scheduler won't uneccessarily context-switch to it.
  g_pLoadThread = ThreadFactory::Create();
  g_pLoadThread->Start();

  // queue up the stuff that will be loaded in the load thread.
  // we call these "WorkItems"
  WorkItemLoadAudio* pLoadOgg = new WorkItemLoadAudio(
      g_pAudioEngine, _X("music/Kefka - NinjaGaiden - Evading the Enemy-loop.ogg"),
      AudioCategory::Music, AudioFormat::Ogg, 18060);

  //WorkItemLoadAudio* pLoadOgg = new WorkItemLoadAudio(
  //    g_pAudioEngine, _X("003 - Grandpa's Theme-loop.ogg"),
  //    AudioCategory::Music, AudioFormat::Ogg);

  WorkItemLoadAudio* pLoadJumpSFX = new WorkItemLoadAudio(
      g_pAudioEngine, _X("sound/jump001.ogg"), AudioCategory::Sound,
      AudioFormat::Ogg, 0, 3);

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

  if (!pWindow_->ShowWindow(SW_SHOWNORMAL)) {
    return false;
  }

  return true;
}

bool ManaGame::OnStartGameLoop() {
  // Run the Windows message loop in this main thread,
  // while we kick off the GameLoopThread which runs
  // our main game loop logic/update/rendering.
  // We keep these in separate threads because we want
  // to avoid things in the main thread from preventing
  // our game loop from being called,
  // such as when the window is begin moved.

  gameThread.Init(GetWindow()->GetHWnd());
  gameThread.Start();

  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  gameThread.Stop();
  gameThread.Join();

  nReturnCode_ = (int)msg.wParam;
  return true;
}

bool ManaGame::OnShutdown() {
  // shutdown engine systems in reverse order to prevent deadlocks

  g_pLoadThread->Stop();
  g_pLoadThread->Join();
  delete g_pLoadThread;
  g_pLoadThread = nullptr;

  g_pAudioEngine->Uninit();
  delete g_pAudioEngine;
  g_pAudioEngine = nullptr;

  g_pInputEngine->Uninit();
  delete g_pInputEngine;
  g_pInputEngine = nullptr;

  g_pEventMan->Uninit();
  delete g_pEventMan;
  g_pEventMan = nullptr;

  ComInitilizer::Uninit();

  return true;
}

}  // namespace Mana

// TODO: remove after doing the below test.
typedef int (*PFN_GETGLOBALINT)(int*);
PFN_GETGLOBALINT pfnGetGlobalInt;
__declspec(dllexport) int* g_pInt = nullptr;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR lpCmdLine,
                      _In_ int nCmdShow) {
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);

  // Prevent multiple instances (within the same session - "Local\")
  // Change the name after the slash for each game.
  Mana::ScopedNamedMutex singleInstance(
      _X("Local\\overworldsoft_unnamed_arpg"));
  if (!singleInstance.TryLock()) {
    Mana::SimpleMessageBox::Show(
        _X("Unnamed ARPG"),
        _X("Game is already running. This instance will close."));
    return 1;
  }

  //ManaLogInit("ManaLog.txt");

  g_pInt = new int(42);
  // TODO: load the dll and try to return the value of g_pInt to see if it can see it!
  HMODULE hLib = ::LoadLibraryW(L"ManaDirectX11.dll");
  if (!hLib) {
    return 1;
  }

  pfnGetGlobalInt = (PFN_GETGLOBALINT)::GetProcAddress(hLib, "GetGlobalInt");
  if (!pfnGetGlobalInt) {
    return 1;
  }
  int val = pfnGetGlobalInt(g_pInt);
  int val2 = *g_pInt;

  g_pGame = new Mana::ManaGame(hInstance, nCmdShow);
  if (!g_pGame->Run(0, nullptr, _X("Untitled Game"))) {
    Mana::SimpleMessageBox::Show(_X("Error"), g_pGame->GetLastError().c_str());
    return 1;
  }

  int returnCode = g_pGame->GetReturnCode();
  delete g_pGame;
  g_pGame = nullptr;
  return returnCode;
}

LRESULT CALLBACK WndProc(HWND hWnd,
                         UINT message,
                         WPARAM wParam,
                         LPARAM lParam) {
  switch (message) {
    case WM_MOUSEMOVE: {
      ((Mana::InputWin*)Mana::g_pInputEngine)->OnMouseMove(wParam, lParam);
    } break;
    case WM_MOUSEWHEEL: {
      // TODO: implement
    } break;
    // Handled for Raw Input API
    // This should only be handling keyboard devices
    case WM_INPUT: {
      OutputDebugStringW(L"WM_INPUT\n");
      ((Mana::InputWin*)Mana::g_pInputEngine)->OnRawInput((HRAWINPUT)lParam);
      return DefWindowProc(hWnd, message, wParam, lParam);
    } break;
    // Handled for Raw Input API
    // This should only be handling keyboard devices
    case WM_INPUT_DEVICE_CHANGE: {
      Mana::U64 deviceId = (Mana::U64)(HANDLE)lParam;
      Mana::InputDeviceChangeType deviceChangeType =
          Mana::InputDeviceChangeType::None;
      if (wParam == GIDC_ARRIVAL)
        deviceChangeType = Mana::InputDeviceChangeType::Added;
      else if (wParam == GIDC_REMOVAL)
        deviceChangeType = Mana::InputDeviceChangeType::Removed;

#ifndef NDEBUG
      wchar_t msg[100];
      swprintf_s(msg, L"WM_INPUT_DEVICE_CHANGE: %s: %llu\n",
                 deviceChangeType == Mana::InputDeviceChangeType::Added
                     ? L"added"
                     : L"removed",
                 deviceId);
      OutputDebugStringW(msg);
#endif

      Mana::g_pInputEngine->OnInputDeviceChange(deviceChangeType, deviceId);
    } break;
    case WM_MENUCHAR: {
      // ignore message beep from alt-enter
      return MNC_CLOSE << 16;
    } break;
    case WM_SYSKEYUP:
    case WM_KEYUP: {
      OutputDebugStringW(L"keyup\n");

      if (wParam == VK_RETURN)  // enter
      {
        // if ((HIWORD(lParam) & KF_ALTDOWN))
        if (GetKeyState(VK_MENU))  // alt-enter
        {
          if (((Mana::ManaGame*)g_pGame)
                  ->GetWindow()
                  ->ToggleFullscreenWindowed()) {
            // OutputDebugStringW(L"alt-enter\n");
          }
        }
      } else if (wParam == VK_UP) {
        //Mana::g_pAudioEngine->Play(oggFile);
        Mana::g_pAudioEngine->Play(oggFile, Mana::AudioBase::LOOP_INFINITE);
      } else if (wParam == VK_DOWN) {
        //Mana::g_pAudioEngine->Stop(jumpSFX);
        Mana::g_pAudioEngine->Stop(oggFile);
      } else if (wParam == VK_LEFT) {
        //Mana::g_pAudioEngine->Pause(jumpSFX);
        Mana::g_pAudioEngine->Pause(oggFile);
      } else if (wParam == VK_RIGHT) {
        //Mana::g_pAudioEngine->Resume(jumpSFX);
        Mana::g_pAudioEngine->Resume(oggFile);
      }
    } break;
    case WM_CHAR: {
      wchar_t msg[50];
      swprintf_s(msg, L"WM_CHAR: %c\n", (wchar_t)wParam);
      OutputDebugStringW(msg);

      wchar_t chr = (wchar_t)wParam;
      if (chr == L'j') {
        Mana::g_pAudioEngine->Play(jumpSFX);
      } else if (chr == L'g') {
        float jumpVolume = Mana::g_pAudioEngine->GetVolume(jumpSFX);

        swprintf_s(msg, L"jumpSFX volume: %.6f\n", jumpVolume);
        OutputDebugStringW(msg);
      } else if (chr == L'v') {
        g_masterVolume = Mana::g_pAudioEngine->GetMasterVolume();

        swprintf_s(msg, L"MasterVolume: %.6f\n", g_masterVolume);
        OutputDebugStringW(msg);
      } else if (chr == L'a')  // volume up
      {
        g_masterVolume += VolumeIncrement;

        swprintf_s(msg, L"SetMasterVolume UP: %.6f\n", g_masterVolume);
        OutputDebugStringW(msg);

        Mana::g_pAudioEngine->SetMasterVolume(g_masterVolume);
      } else if (chr == L'z')  // volume down
      {
        g_masterVolume -= VolumeIncrement;

        swprintf_s(msg, L"SetMasterVolume DOWN: %.6f\n", g_masterVolume);
        OutputDebugStringW(msg);

        Mana::g_pAudioEngine->SetMasterVolume(g_masterVolume);
      } else if (chr == L'q')  // pan left
      {
        float pan = Mana::g_pAudioEngine->GetPan(jumpSFX);
        pan -= PanIncrement;
        Mana::g_pAudioEngine->SetPan(jumpSFX, pan);
      } else if (chr == L'w')  // pan center
      {
        Mana::g_pAudioEngine->SetPan(jumpSFX, 0.0f);
      } else if (chr == L'e')  // pan right
      {
        float pan = Mana::g_pAudioEngine->GetPan(jumpSFX);
        pan += PanIncrement;
        Mana::g_pAudioEngine->SetPan(jumpSFX, pan);
      }
    } break;
    case WM_CLOSE: {
      DestroyWindow(hWnd);
    } break;
    case WM_PAINT: {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hWnd, &ps);
      Mana::Debug_DrawText_GDI(
          hWnd, hdc, 40, 10,
          std::wstring(L"FPS: ") + std::to_wstring(Mana::g_fps)
        );
      EndPaint(hWnd, &ps);
    } break;
    case WM_DESTROY:
      PostQuitMessage(0);
      break;
    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
  }
  return 0;
}
