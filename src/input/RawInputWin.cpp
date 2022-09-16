#include "pch.h"

#include "input/RawInputWin.h"

#include <cassert>
#include <strsafe.h>
#include "events/EventManager.h"
#include "input/DualSenseWin.h"
#include "input/DualShock4Win.h"
#include "input/InputBase.h"
#include "input/InputWin.h"

namespace Mana {

RawInputWin::RawInputWin(HWND hwndTarget)
    : hwndTarget_(hwndTarget),
      pRawInput_(nullptr), rawInputSizeBytes_(96) {}
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
  // from keyboards and gamepads

  RAWINPUTDEVICE rid[2];

  rid[0].usUsagePage = 0x01;
  rid[0].usUsage = 0x06;  // keyboards
  // TODO: possibly specify RIDEV_DEVNOTIFY. See: https://docs.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-rawinputdevice
  rid[0].dwFlags = 0;
  rid[0].hwndTarget = hwndTarget_;

  rid[1].usUsagePage = 0x01;
  rid[1].usUsage = 0x05;  // gamepads
  // TODO: possibly specify RIDEV_DEVNOTIFY. See: https://docs.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-rawinputdevice
  rid[1].dwFlags = 0;
  rid[1].hwndTarget = hwndTarget_;

  // after registering, the WinProc will get WM_INPUT messages
  if (!RegisterRawInputDevices(rid, 2, sizeof(rid[0]))) {
    return false;
  }

  return true;
}

bool RawInputWin::UnregisterDevices() {
  RAWINPUTDEVICE rid[2];

  rid[0].usUsagePage = 0x01;
  rid[0].usUsage = 0x06; // keyboards
  rid[0].dwFlags = RIDEV_REMOVE;
  rid[0].hwndTarget = hwndTarget_;

  rid[1].usUsagePage = 0x01;
  rid[1].usUsage = 0x05; // gamepads
  rid[1].dwFlags = RIDEV_REMOVE;
  rid[1].hwndTarget = hwndTarget_;

  if (!RegisterRawInputDevices(rid, 2, sizeof(rid[0]))) {
    return false;
  }

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
    wchar_t buffer[256];
    StringCchPrintfW(buffer, ARRAYSIZE(buffer),
                     L"!!!!!!!! RAW INPUT REALLOC !!!!!!!! dataSize=%04u\n",
                     dataSize);
    OutputDebugStringW(buffer);
    //assert(false && "RAW INPUT REALLOC !!");
    if (!ReallocRawInputPtr(dataSize)) {
      return false;
    }
  }

  GetRawInputData(hRawInput, RID_INPUT, pRawInput_, &dataSize,
                  sizeof(RAWINPUTHEADER));

  const RAWINPUT* input = (const RAWINPUT*)pRawInput_;

  // ==============================================================
  // --- keyboard -------------------------------------------------
  if (input->header.dwType == RIM_TYPEKEYBOARD) {

#ifndef NDEBUG
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
#endif  // DEBUG

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

  // ==============================================================
  // --- gamepad / other ------------------------------------------
  } else if (input->header.dwType == RIM_TYPEHID) { // not keyboard nor mouse
    // probably a gamepad.
    // get more device info so we can check

    RID_DEVICE_INFO deviceInfo;
    UINT deviceInfoSize = sizeof(deviceInfo);
    bool gotInfo =
        GetRawInputDeviceInfoW(input->header.hDevice, RIDI_DEVICEINFO,
                               &deviceInfo, &deviceInfoSize) > 0;

    // TODO: maybe move deviceName so it's reused and not on the stack?
    WCHAR deviceName[1024] = {0};
    UINT deviceNameLength = sizeof(deviceName) / sizeof(*deviceName);
    bool gotName =
        GetRawInputDeviceInfoW(input->header.hDevice, RIDI_DEVICENAME,
                               deviceName, &deviceNameLength) > 0;

    if (gotInfo && gotName) {

      // TODO: output the following. Make sure deviceName is unique for multiple DS4 controllers, else find a way to support multiple.
      //deviceInfo.hid.dwVendorId
      //deviceInfo.hid.dwProductId
      //deviceName

      if (IsDualShock4(deviceInfo.hid)) {

        // TODO: use "input->header.hDevice" to see if this is a new device
        //       that isn't assigned to a player yet.
        //       Only allow one controller per player.

        SynchronizedEvent syncEvent;
        syncEvent.syncEventType = (U8)SynchronizedEventType::Input;

        if (!GetStateDualShock4(hwndTarget_, input->data.hid.bRawData,
                                 input->data.hid.dwSizeHid, deviceName,
                                 syncEvent.inputAction)) {
          return false;
        }

        g_pEventMan->EnqueueForGameLoop(syncEvent);

      } else if (IsDualSense(deviceInfo.hid)) {
        // TODO: use "input->header.hDevice" to see if this is a new device
        //       that isn't assigned to a player yet.
        //       Only allow one controller per player.

        SynchronizedEvent syncEvent;
        syncEvent.syncEventType = (U8)SynchronizedEventType::Input;

        if (!GetStateDualSense(hwndTarget_, input->data.hid.bRawData,
                               input->data.hid.dwSizeHid, deviceName,
                               syncEvent.inputAction)) {
          return false;
        }

        g_pEventMan->EnqueueForGameLoop(syncEvent);
      }
    }
  }

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
