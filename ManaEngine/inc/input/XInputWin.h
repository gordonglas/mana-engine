#pragma once

#include "ManaGlobals.h"
#include "target/TargetOS.h"
#include "utils/Timer.h"
#include <xinput.h>

namespace Mana {

struct GamepadTimer {
  bool isConnected;
  DWORD packetNumber;
  uint64_t previous;
  uint64_t current;
  uint64_t elapsed;
  uint64_t total;
};

// XInput 1.4 wrapper - For Xbox (or Xbox-like) gamepads.
// This is meant only to be used internally by the game engine.
// XInput 1.4 ships as a system component in Windows 8 and up,
// and does not require redistribution with an application.
// Windows SDK contains the header and import library for
// statically linking against XINPUT1_4.DLL.
class XInput {
 public:
  XInput();
  virtual ~XInput() = default;

  XInput(const XInput&) = delete;
  XInput& operator=(const XInput&) = delete;

  XINPUT_STATE state_[4] = {};

  // Essentially calls XInputGetState.
  // It's safe to call this every frame.
  // If a gamepad isn't connected, it will only actually allow a call to
  // XInputGetState every second or so, for performance as per XInput docs.
  // |id| is always 0 through 3 and corresponds to a specific port.
  DWORD GetGamepadState(U8 id, XINPUT_STATE* state);

  // TODO: check XInputGetCapabilities
  // see: https://learn.microsoft.com/en-us/windows/win32/api/xinput/nf-xinput-xinputgetcapabilities

  // TODO: support for XInputGetBatteryInformation
  // see: https://learn.microsoft.com/en-us/windows/win32/api/xinput/nf-xinput-xinputgetbatteryinformation

 private:
  Timer clock_;
  GamepadTimer timer_[4] = {};
};

} // namespace Mana
