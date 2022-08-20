#include "pch.h"

#include "input/RawInputWin.h"

#include <strsafe.h>
#include "events/EventManager.h"
#include "input/InputBase.h"
#include "input/InputWin.h"

namespace Mana {

RawInputWin::RawInputWin(HWND hwndTarget)
    : hwndTarget_(hwndTarget),
      pRawInput_(nullptr), rawInputSizeBytes_(80) {}
RawInputWin::~RawInputWin() {}

bool RawInputWin::Init() {
  // allocate dynamic memory for RAWINPUT type used at run-time.
  pRawInput_ = (RAWINPUT*)malloc(rawInputSizeBytes_);

  return RegisterDevices();
}

bool RawInputWin::Uninit() {
  UnregisterDevices();

  if (pRawInput_) {
    free(pRawInput_);
    pRawInput_ = nullptr;
  }

  return true;
}

bool RawInputWin::RegisterDevices() {
  // allow our game to recieve raw input
  // from multiple keyboards and mice

  RAWINPUTDEVICE keyboards;
  keyboards.usUsagePage = 1;
  keyboards.usUsage = 6;
  keyboards.dwFlags = 0;
  keyboards.hwndTarget = hwndTarget_;

  // after registering it, the WinProc will get WM_INPUT messages
  if (!RegisterRawInputDevices(&keyboards, 1, sizeof(keyboards))) {
    return false;
  }

  /*RAWINPUTDEVICE mice;
  mice.usUsagePage = 1;
  mice.usUsage = 2;
  mice.dwFlags = 0;
  mice.hwndTarget = hwndTarget_;

  if (!RegisterRawInputDevices(&mice, 1, sizeof(mice))) {
    return false;
  }*/

  return true;
}

bool RawInputWin::UnregisterDevices() {
  RAWINPUTDEVICE keyboards;
  keyboards.usUsagePage = 1;
  keyboards.usUsage = 6;
  keyboards.dwFlags = RIDEV_REMOVE;
  keyboards.hwndTarget = hwndTarget_;

  if (!RegisterRawInputDevices(&keyboards, 1, sizeof(keyboards))) {
    return false;
  }

  /*RAWINPUTDEVICE mice;
  mice.usUsagePage = 1;
  mice.usUsage = 2;
  mice.dwFlags = RIDEV_REMOVE;
  mice.hwndTarget = hwndTarget_;

  if (!RegisterRawInputDevices(&mice, 1, sizeof(mice))) {
    return false;
  }*/

  return true;
}

bool RawInputWin::OnRawInput(HRAWINPUT hRawInput) {
  UINT dataSize;
  if (GetRawInputData(hRawInput, RID_INPUT, nullptr, &dataSize,
                      sizeof(RAWINPUTHEADER)) != 0) {
    return false;
  }

  if (dataSize == 0)
    return false;
  if (dataSize > rawInputSizeBytes_) {
    // this is ok, just want to make sure it doesn't happen often.
    // if it does happen often, adjust initial value for rawInputSizeBytes_
    OutputDebugStringW(L"!!!!!!!! RAW INPUT REALLOC !!!!!!!!\n");
    if (!ReallocRawInputPtr(dataSize)) {
      return false;
    }
  }

  GetRawInputData(hRawInput, RID_INPUT, pRawInput_, &dataSize,
                  sizeof(RAWINPUTHEADER));

  const RAWINPUT* input = (const RAWINPUT*)pRawInput_;
  if (input->header.dwType == RIM_TYPEKEYBOARD) {
    wchar_t prefix[80];
    prefix[0] = L'\0';
    if (input->data.keyboard.Flags & RI_KEY_E0) {
      StringCchCatW(prefix, ARRAYSIZE(prefix), L"E0 ");
    }
    if (input->data.keyboard.Flags & RI_KEY_E1) {
      StringCchCatW(prefix, ARRAYSIZE(prefix), L"E1 ");
    }

    wchar_t buffer[256];
    StringCchPrintfW(
        buffer, ARRAYSIZE(buffer),
        L"%p, msg=%04x, vk=%04x, scanCode=%s%02x, %s\n",
        input->header.hDevice, input->data.keyboard.Message,
        input->data.keyboard.VKey, prefix, input->data.keyboard.MakeCode,
        (input->data.keyboard.Flags & RI_KEY_BREAK) ? L"release"
                                                    : L"press");

    OutputDebugStringW(buffer);

    // wrap InputAction into a SyncronizedEvent and pass
    // to the SynchronizedQueue used to send it to the game-loop thread.
    SynchronizedEvent syncEvent;
    syncEvent.syncEventType = (U8)SynchronizedEventType::Input;

    InputAction& action = syncEvent.inputAction;
    action.deviceType = (U8)InputDeviceType::Keyboard;
    action.deviceId = (U64)input->header.hDevice;
    action.virtualKey = input->data.keyboard.VKey;
    action.scanCode = input->data.keyboard.MakeCode;
    action.flags =
        ((input->data.keyboard.Flags & RI_KEY_E0) ? INPUTACTION_FLAG_E0 : 0) |
        ((input->data.keyboard.Flags & RI_KEY_E1) ? INPUTACTION_FLAG_E1 : 0) |
        ((input->data.keyboard.Flags & RI_KEY_BREAK) ? INPUTACTION_FLAG_RELEASE
                                                     : 0);

    g_pEventMan->EnqueueForGameLoop(syncEvent);

  } //else if (input->header.dwType == RIM_TYPEMOUSE) {
  //
  //}

  return true;
}

bool RawInputWin::ReallocRawInputPtr(size_t size) {
  void* ptr = realloc(pRawInput_, size);
  if (!ptr)
    return false;
  pRawInput_ = ptr;
  rawInputSizeBytes_ = size;
  return true;
}

}  // namespace Mana
