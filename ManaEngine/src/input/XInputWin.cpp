#include "pch.h"
#include "input/XInputWin.h"
#include <cassert>

namespace Mana {

XInput::XInput() {
  clock_.Reset();
  uint64_t time = clock_.GetMicroseconds();
  for (size_t i = 0; i < 4; i++) {
    timer_[i].previous = time;
    // so first call to GetGamepadState will call XInputGetState
    timer_[i].total = 1000000;
  }
}

XInput::~XInput() {}

DWORD XInput::GetGamepadState(U8 id, XINPUT_STATE* state) {
  assert(id >= 0 && id < 4);

  DWORD result = ERROR_DEVICE_NOT_CONNECTED;

  GamepadTimer& t = timer_[id];

  if (t.isConnected) {
    // attempt to get state

    // save the packet number and reset fields
    t.packetNumber = state->dwPacketNumber;
    *state = {};

    result = ::XInputGetState(id, state);
    if (result != ERROR_SUCCESS) {
      t.isConnected = false;
      t.previous = clock_.GetMicroseconds();
      t.total = 0;
    }
  } else {
    t.current = clock_.GetMicroseconds();
    t.elapsed = t.current - t.previous;
    if (t.elapsed < 0) {
      t.elapsed = 0;
    }
    t.total += t.elapsed;

    // if 1 second elapsed, attempt to get state
    if (t.total >= 1000000) {
      // reset fields
      t.packetNumber = 0;
      *state = {};

      result = ::XInputGetState(id, state);
      if (result == ERROR_SUCCESS) {
        t.isConnected = true;
        t.previous = clock_.GetMicroseconds();
      }

      t.total = 0;
    }
  }

  return result;
}

} // namespace Mana
