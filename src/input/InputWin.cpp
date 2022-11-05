#include "pch.h"
#include "input/InputWin.h"
#include <Windowsx.h> // GET_X_LPARAM, GET_Y_LPARAM
#include "events/EventManager.h"

namespace Mana {

InputBase* g_pInputEngine = nullptr;

InputWin::InputWin(HWND hwnd) : hwnd_(hwnd), pRawInput_(nullptr) {}
InputWin::~InputWin() {}

bool InputWin::Init() {
  pRawInput_ = new RawInputWin(hwnd_);
  if (!pRawInput_)
    return false;
  if (!pRawInput_->Init())
    return false;
  return true;
}

void InputWin::Uninit() {
  if (pRawInput_) {
    pRawInput_->Uninit();
    delete pRawInput_;
    pRawInput_ = nullptr;
  }
}

bool InputWin::OnRawInput(HRAWINPUT hRawInput) {
  return pRawInput_->OnRawInput(hRawInput);
}

bool InputWin::OnInputDeviceChange(InputDeviceChangeType deviceChangeType,
                                   U64 deviceId) {
  return pRawInput_->OnInputDeviceChange(deviceChangeType, deviceId);
}

void InputWin::OnMouseMove(WPARAM wParam, LPARAM lParam) {
  // wrap InputAction into a SyncronizedEvent and pass
  // to the SynchronizedQueue used to send it to the game-loop thread.
  SynchronizedEvent syncEvent;
  syncEvent.syncEventType = (U8)SynchronizedEventType::Input;

  InputAction& action = syncEvent.inputAction;
  action.deviceType = (U8)InputDeviceType::Mouse;

  // Only Raw Input API can differentiate multiple mouse devices,
  // but we don't care and are using WM_MOUSEMOVE messages.
  action.deviceId = 0;

  // not using keyboard fields
  action.virtualKey = 0;
  action.scanCode = 0;
  action.flags = 0;

  // set mouse fields.
  // Using same bitfield as wParam from WM_MOUSEMOVE status:
  // See: https://docs.microsoft.com/en-us/windows/win32/inputdev/wm-mousemove#parameters
  action.wParam = wParam;
  // Note: mouse pos can be negative on multi-monitor setups
  action.mouseX = GET_X_LPARAM(lParam);
  action.mouseY = GET_Y_LPARAM(lParam);

  g_pEventMan->EnqueueForGameLoop(syncEvent);
}

}  // namespace Mana
