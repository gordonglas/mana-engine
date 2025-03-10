#include "pch.h"

#include "input/RawInputWin.h"

#include <cassert>
#include <strsafe.h>
#include "events/EventManager.h"
#include "input/InputBase.h"
#include "input/InputWin.h"

namespace Mana {

RawInputWin::RawInputWin(HWND hwndTarget)
    : hwndTarget_(hwndTarget),
      pRawInput_(nullptr), rawInputSizeBytes_(80) {}

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
  // from keyboards

  RAWINPUTDEVICE rid[1];

  rid[0].usUsagePage = 0x01;
  rid[0].usUsage = 0x06;  // keyboards
  // register for device add/remove notifications via WM_INPUT_DEVICE_CHANGE
  rid[0].dwFlags = RIDEV_DEVNOTIFY;
  rid[0].hwndTarget = hwndTarget_;

  // after registering, the WinProc will get WM_INPUT and
  // WM_INPUT_DEVICE_CHANGE messages
  if (!RegisterRawInputDevices(rid, 1, sizeof(RAWINPUTDEVICE))) {
    return false;
  }

  return true;
}

bool RawInputWin::UnregisterDevices() {
  RAWINPUTDEVICE rid[1];

  rid[0].usUsagePage = 0x01;
  rid[0].usUsage = 0x06; // keyboards
  rid[0].dwFlags = RIDEV_REMOVE;
  rid[0].hwndTarget = hwndTarget_;

  if (!RegisterRawInputDevices(rid, 1, sizeof(RAWINPUTDEVICE))) {
    return false;
  }

  return true;
}

// We actually DO get a OnInputDeviceChange->DeviceAdded event for
// each device that's already attached to the computer, when the
// app first calls RegisterRawInputDevices.
// So we don't have to put any "DeviceAdded" checks within OnRawInput.
// TODO: make sure the above comment's logic is consistent with
//       other forms of input devices, such as XInput.
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
  }

  return true;
}

// NOTE: Some devices don't work well with Raw Input API OnInputDeviceChange
// notifications, such as Nintendo Switch Pro controllers, so we can only
// really use OnInputDeviceChange for keyboards. We use XInput for gamepads
// anyway, so it's not a problem.
bool RawInputWin::OnInputDeviceChange(InputDeviceChangeType deviceChangeType,
                                      U64 deviceId) {
  assert(deviceId > 0 && "null deviceId!!!");

  if (deviceChangeType == InputDeviceChangeType::Removed) {
    // for devices that were removed, we still get it's deviceId,
    // so the engine can use it (soon, not here) to remove
    // it from the engine.

    SynchronizedEvent syncEvent;
    syncEvent.syncEventType = (U8)SynchronizedEventType::InputDeviceChange;
    
    InputAction& action = syncEvent.inputAction;
    action.deviceId = deviceId;
    action.deviceChangeType = (U8)deviceChangeType;

    g_pEventMan->EnqueueForGameLoop(syncEvent);
  }

  HANDLE hDevice = (HANDLE)deviceId;

  RID_DEVICE_INFO deviceInfo;
  UINT deviceInfoSize = sizeof(deviceInfo);
  bool gotInfo = GetRawInputDeviceInfoW(hDevice, RIDI_DEVICEINFO,
                                        &deviceInfo, &deviceInfoSize) > 0;

  WCHAR deviceName[1024] = {0};
  UINT deviceNameLength = sizeof(deviceName) / sizeof(*deviceName);
  bool gotName = GetRawInputDeviceInfoW(hDevice, RIDI_DEVICENAME,
                                        deviceName, &deviceNameLength) > 0;

  if (gotInfo && gotName) {
    SynchronizedEvent syncEvent;
    syncEvent.syncEventType = (U8)SynchronizedEventType::InputDeviceChange;

    InputAction& action = syncEvent.inputAction;
    action.deviceId = deviceId;
    action.deviceChangeType = (U8)deviceChangeType;

    // get device type
    InputDeviceType deviceType = InputDeviceType::Unknown;
    switch (deviceInfo.dwType) {
      case RIM_TYPEKEYBOARD: {
        deviceType = InputDeviceType::Keyboard;
      } break;
      //case RIM_TYPEMOUSE: {
      //  deviceType = InputDeviceType::Mouse;
      //} break;
      default:
        // ignore other device types
        return true;
    }
    action.deviceType = (U8)deviceType;

#ifndef NDEBUG
    std::string sDeviceChangeType("None");
    if (deviceChangeType == InputDeviceChangeType::Added)
      sDeviceChangeType = "Added";
    else if (deviceChangeType == InputDeviceChangeType::Removed)
      sDeviceChangeType = "Removed";

    std::string sDeviceType("Unknown");
    if (deviceType == InputDeviceType::Keyboard)
      sDeviceType = "Keyboard";
    //else if (deviceType == InputDeviceType::Mouse)
    //  sDeviceType = "Mouse";

    wchar_t buffer[256];
    StringCchPrintfW(buffer, ARRAYSIZE(buffer),
                     L"Device %S, type:%S, hDevice: %p, name=%s\n",
                     sDeviceChangeType.c_str(), sDeviceType.c_str(), hDevice,
                     deviceName);
    OutputDebugStringW(buffer);
#endif

    g_pEventMan->EnqueueForGameLoop(syncEvent);
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
