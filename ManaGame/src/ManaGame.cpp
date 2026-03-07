#include "Globals.h"
#include "target/TargetOS.h"
#include <atomic>
#include <optional>
#include <shellapi.h>
#include <string.h>
#include "concurrency/NamedMutex.h"
#include "concurrency/ThreadRunnerWin.h"
#include "config/ConfigManager.h"
#include "debugging/DebugWin.h"  // Debug_DrawText_GDI
#include "events/EventManager.h"
#include "GameThread.h"
#include "input/InputWin.h"
#include "mainloop/ManaGameBase.h"
#include "os/WindowWin.h"
#include "ui/SimpleMessageBox.h"
#include "utils/CommandLine.h"
#include "utils/ScopedComInitializer.h"
#include "utils/Strings.h"

Mana::xstring title(_X("Unnamed ARPG"));

#define MAX_LOADSTRING 100

WCHAR szTitle[MAX_LOADSTRING];        // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];  // the main window class name

// Forward declarations
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

namespace Mana {

class ManaGame : public ManaGameBase {
 public:
  ManaGame(HINSTANCE hInstance, int nCmdShow)
      : hInstance_(hInstance), nCmdShow_(nCmdShow), threadRunner_(nullptr) {
    threadRunner_ = new ThreadRunnerWin();
  }

  ~ManaGame() final {
    if (threadRunner_) {
      delete threadRunner_;
      threadRunner_ = nullptr;
    }
  }

  //uint64_t GetFps() final { return g_fps; }

  WindowWin* GetWindow() { return (WindowWin*)pWindow_; }
  ThreadRunnerWin* GetThreadRunner() {
    return static_cast<ThreadRunnerWin*>(threadRunner_);
  }

 protected:
  bool OnInit() final;
  bool OnStartGameThread() final;
  bool OnRunMessageLoop() final;
  bool OnShutdown() final;

 private:
  ScopedComInitializer com_;
  HINSTANCE hInstance_;
  int nCmdShow_;
  ThreadRunnerBase* threadRunner_;
};

bool ManaGame::OnInit() {
  // TODO: is com even needed here? probably not?
  if (!com_.Init()) {
    error_ = _X("ComInitializer error");
    return false;
  }

  ManaGameBase::OnInit();

  // TODO: init ConfigManager and load game configs

  pWindow_ = new WindowWin(hInstance_, nCmdShow_, WndProc);
  if (!pWindow_->CreateMainWindow(commandLine_, title_)) {
    error_ = _X("CreateMainWindow error");
    return false;
  }

  threadRunner_->SetWindow(static_cast<WindowWin*>(pWindow_));

  // init event manager
  g_pEventMan = new EventManager();
  g_pEventMan->Init();

  // init keyboard and mouse input engine
  g_pInputEngine = new InputWin(GetWindow()->GetHWnd());
  g_pInputEngine->Init();

  if (!pWindow_->ShowWindow(SW_SHOWNORMAL)) {
    return false;
  }

  return true;
}

bool ManaGame::OnStartGameThread() {
  // Run game loop logic/update/rendering in a separate thread,
  // to avoid things in the main thread from preventing the game loop
  // from being called, such as when the window is begin moved.
  gameThread_ = new GameThread(*GetWindow(), *GetThreadRunner());
  if (!gameThread_) {
    return false;
  }

  if (!gameThread_->Run()) {
    return false;
  }

  return true;
}

bool ManaGame::OnRunMessageLoop() {
  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  nReturnCode_ = (int)msg.wParam;
  return true;
}

bool ManaGame::OnShutdown() {
  // Shutdown engine systems in reverse order to prevent deadlocks

  if (gameThread_) {
    gameThread_->OnShutdown();
    delete gameThread_;
    gameThread_ = nullptr;
  }

  if (g_pInputEngine) {
    g_pInputEngine->Uninit();
    delete g_pInputEngine;
    g_pInputEngine = nullptr;
  }

  if (g_pEventMan) {
    g_pEventMan->Uninit();
    delete g_pEventMan;
    g_pEventMan = nullptr;
  }

  if (pWindow_) {
    delete pWindow_;
    pWindow_ = nullptr;
  }

  return true;
}

}  // namespace Mana

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
        title.c_str(),
        _X("Game is already running. This instance will close."));
    return 1;
  }

  ManaLogInit("ManaLog.txt");

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
  // TODO: Most keys/events should be mapped by the InputEngine,
  //       and sent to the GameThread via EventManager.
  switch (message) {
    case WM_MOUSEMOVE: {
      // TODO: uncomment
      //((Mana::InputWin*)Mana::g_pInputEngine)->OnMouseMove(wParam, lParam);
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
    // Allows other non-main threads (AKA the Game Thread) to run lambdas
    // synchronously on this main thread.
    case WM_RUN_ON_MAIN_THREAD: {
      Mana::ManaGame* game = (Mana::ManaGame*)g_pGame;
      auto& queue = game->GetThreadRunner()->GetSyncQueue();
      auto func = queue.Pop();
      if (func) {
        (func.value())();
      }
    } break;
    // Allows other non-main threads (AKA the Game Thread) to run lambdas
    // asynchronously on this main thread.
    case WM_RUN_ON_MAIN_THREAD_ASYNC: {
      Mana::ManaGame* game = (Mana::ManaGame*)g_pGame;
      auto& queue = game->GetThreadRunner()->GetAsyncQueue();
      auto func = queue.Pop();
      if (func) {
        (func.value())();
      }
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
      }
      // TODO: Get these working in GameThread
      /*
      else if (wParam == VK_UP) {
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
      }*/
    } break;
    case WM_CHAR: {
      wchar_t msg[50];
      swprintf_s(msg, L"WM_CHAR: %c\n", (wchar_t)wParam);
      OutputDebugStringW(msg);

      // TODO: Get these working in GameThread
      /*
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
      } else if (chr == L'd')  // debug: dx11 ReportLiveObjects
      {
        DXDBG_REPORT_LIVE_OBJECTS(Mana::g_pGraphicsEngine);
      }
      */
    } break;
    case WM_CLOSE: {
      DestroyWindow(hWnd);
    } break;
    case WM_PAINT: {
      // TODO: move this into GameThread, using DirectX, once it's up.
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
