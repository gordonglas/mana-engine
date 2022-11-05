#include "pch.h"

#include "input/DualSenseWin.h"

#include <strsafe.h>

namespace Mana {

bool IsDualSense(const RID_DEVICE_INFO_HID& info) {
  // https://github.com/nondebug/dualsense
  const DWORD sonyVendorID = 0x054C;
  const DWORD dualSenseGen1ProductID = 0x0CE6;
  // TODO: also verify these different ids. Maybe they are from info.dwVersionNumber???
  //       https://playstationdev.wiki/ps5devwiki/index.php/DualSense

  return info.dwVendorId == sonyVendorID &&
         (info.dwProductId == dualSenseGen1ProductID);
}

bool GetStateDualSense(HWND hWnd,
                         const BYTE rawData[],
                         DWORD byteCount,
                         WCHAR* deviceName,
                         InputAction& inputAction) {
  // Input Report format:
  // https://github.com/nondebug/dualsense#input-reports
  // https://controllers.fandom.com/wiki/Sony_DualSense

  //const DWORD usbInputByteCount = 64;
  //// TODO: THIS MIGHT BE WRONG FOR DUALSENSE!!!! WAS COPIED FROM DUALSHOCK 4.
  ////       See what byteCount is when connected via Bluetooth.
  //const DWORD bluetoothInputByteCount = 547;

  int offset = 0;
  //if (byteCount == bluetoothInputByteCount) {
  //  offset = 2;
  //}

  // left: 0x00, right: 0xff, neutral: ~0x80
  BYTE leftStickX = rawData[offset + 1];
  // up: 0x00, down: 0xff, neutral: ~0x80
  BYTE leftStickY = rawData[offset + 2];
  // left: 0x00, right: 0xff, neutral: ~0x80
  BYTE rightStickX = rawData[offset + 3];
  // up: 0x00, down: 0xff, neutral: ~0x80
  BYTE rightStickY = rawData[offset + 4];
  // neutral: 0x00, pressed: 0xff
  BYTE leftTrigger = rawData[offset + 5];
  // neutral: 0x00, pressed: 0xff
  BYTE rightTrigger = rawData[offset + 6];

  // byte 7, bit 0: Vendor defined 0xFF00 / 0x20 - 8 bits (unused here)

  // neutral: 0x8, N: 0x0, NE: 0x1, E: 0x2, SE: 0x3, S: 0x4, SW: 0x5, W: 0x6, NW: 0x7
  // 4 bits, starting at bit 0
  BYTE dpad = 0b1111 & rawData[offset + 8];

#ifndef NDEBUG
  wchar_t buffer[256];
  StringCchPrintfW(
      buffer, ARRAYSIZE(buffer),
      L"DS5 - LX:%3d LY:%3d RX:%3d RY:%3d LT:%3d RT:%3d Dpad:%1d\n", leftStickX,
      leftStickY, rightStickX, rightStickY, leftTrigger, rightTrigger, dpad);
  OutputDebugStringW(buffer);
#endif  // DEBUG

  // TODO: Try to find battery info in other unused fields.
  //       Maybe not needed though.
  //BYTE battery = rawData[offset + 12];

  // TODO: doesn't have the touchpad? (can't remember)
//  int16_t touch1X = ((rawData[offset + 37] & 0x0F) << 8) | rawData[offset + 36];
//  int16_t touch1Y = ((rawData[offset + 37]) >> 4) | (rawData[offset + 38] << 4);
//  int16_t touch2X = ((rawData[offset + 41] & 0x0F) << 8) | rawData[offset + 40];
//  int16_t touch2Y = ((rawData[offset + 41]) >> 4) | (rawData[offset + 42] << 4);
//
//#ifndef NDEBUG
//  StringCchPrintfW(
//      buffer, ARRAYSIZE(buffer),
//      //Battery:%3d
//      L"Touch1X:%4d Touch1Y:%4d Touch2X:%4d Touch2Y:%4d\n",
//      //battery,
//      touch1X, touch1Y, touch2X, touch2Y);
//  OutputDebugStringW(buffer);
//#endif  // DEBUG

#ifndef NDEBUG
  StringCchPrintfW(buffer, ARRAYSIZE(buffer), L"Buttons: ");
  if (1 & (rawData[offset + 8] >> 4))
    StringCchCatW(buffer, ARRAYSIZE(buffer), L"Square ");
  if (1 & (rawData[offset + 8] >> 5))
    StringCchCatW(buffer, ARRAYSIZE(buffer), L"X ");
  if (1 & (rawData[offset + 8] >> 6))
    StringCchCatW(buffer, ARRAYSIZE(buffer), L"O ");
  if (1 & (rawData[offset + 8] >> 7))
    StringCchCatW(buffer, ARRAYSIZE(buffer), L"Triangle ");
  if (1 & (rawData[offset + 9] >> 0))
    StringCchCatW(buffer, ARRAYSIZE(buffer), L"L1 ");
  if (1 & (rawData[offset + 9] >> 1))
    StringCchCatW(buffer, ARRAYSIZE(buffer), L"R1 ");
  if (1 & (rawData[offset + 9] >> 2))
    StringCchCatW(buffer, ARRAYSIZE(buffer), L"L2 ");
  if (1 & (rawData[offset + 9] >> 3))
    StringCchCatW(buffer, ARRAYSIZE(buffer), L"R2 ");
  if (1 & (rawData[offset + 9] >> 4))
    StringCchCatW(buffer, ARRAYSIZE(buffer), L"Create ");
  if (1 & (rawData[offset + 9] >> 5))
    StringCchCatW(buffer, ARRAYSIZE(buffer), L"Options ");
  if (1 & (rawData[offset + 9] >> 6))
    StringCchCatW(buffer, ARRAYSIZE(buffer), L"L3 ");
  if (1 & (rawData[offset + 9] >> 7))
    StringCchCatW(buffer, ARRAYSIZE(buffer), L"R3 ");
  if (1 & (rawData[offset + 10] >> 0))
    StringCchCatW(buffer, ARRAYSIZE(buffer), L"PS ");
  if (1 & (rawData[offset + 10] >> 1))
    StringCchCatW(buffer, ARRAYSIZE(buffer), L"TouchPad ");
  if (1 & (rawData[offset + 10] >> 2))
    StringCchCatW(buffer, ARRAYSIZE(buffer), L"Mute ");
  StringCchCatW(buffer, ARRAYSIZE(buffer), L"\n");
  OutputDebugStringW(buffer);
#endif  // DEBUG

  // TODO: HERE!!! finish. Set inputAction fields.
  inputAction.deviceType = (U8)InputDeviceType::Gamepad;
  inputAction.gamepadType = (U8)InputGamepadType::DualSense;

  return true;
}

// TODO: HERE!!!
//       (can we even do this on DualSense???? need some sample code?)
// 
//       (need other functions that set LED color, and set force feedback.)
//       See:
//       https://github.com/MysteriousJ/Joystick-Input-Examples/blob/main/src/specialized.cpp#L46
//       Explained here:
//       https://github.com/MysteriousJ/Joystick-Input-Examples#dualshock-4

// This is only used for setting LED and Force Feedback..
//PropUserData* userData = (PropUserData*)GetPropA(hWnd, "userData");
//if (!userData) {
//  return false;
//}

}  // namespace Mana
