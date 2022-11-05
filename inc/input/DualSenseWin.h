#pragma once

#include "ManaGlobals.h"
#include "target/TargetOS.h"
#include "input/InputBase.h"

namespace Mana {

// PS5 controller
bool IsDualSense(const RID_DEVICE_INFO_HID& info);

// fills |inputAction| with current state
bool GetStateDualSense(HWND hWnd,
                         const BYTE rawData[],
                         DWORD byteCount,
                         WCHAR* deviceName,
                         InputAction& inputAction);
}  // namespace Mana
