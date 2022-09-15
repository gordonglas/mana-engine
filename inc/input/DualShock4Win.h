#pragma once

#include "ManaGlobals.h"
#include "target/TargetOS.h"
#include "input/InputBase.h"

namespace Mana {

// PS4 controller
bool IsDualShock4(RID_DEVICE_INFO_HID info);

// PS5 controller
bool IsDualSense(RID_DEVICE_INFO_HID info);

// fills |inputAction| with current state
bool GetStateDualShock4(HWND hWnd,
                         const BYTE rawData[],
                         DWORD byteCount,
                         WCHAR* deviceName,
                         InputAction& inputAction);
}  // namespace Mana
