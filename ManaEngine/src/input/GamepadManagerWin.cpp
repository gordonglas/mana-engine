#include "pch.h"
#include "input/GamepadManager.h"
#include "input/XInputWin.h"

#include <cassert>

namespace Mana {

XInput* xinput = nullptr;

GamepadManager::GamepadManager() : state_(nullptr) {}

bool GamepadManager::Init() {
  // XInput's gamepad ids are from 0 to 3 inclusive,
  // and any one of the ids could be use (each id corresponds to a port).
  // So games must check for status of each port.
  state_ = new GAMEPAD_STATE[4];
  if (!state_) {
    return false;
  }

  xinput = new XInput();
  if (!xinput) {
    return false;
  }

  return true;
}

void GamepadManager::Uninit() {
  if (state_) {
    delete[] state_;
  }

  if (xinput) {
    delete xinput;
  }
}

// TODO: test. Will need to add lib and dll to projects. See XInputWin.h comments.
GAMEPAD_STATE& GamepadManager::GetGamepadState(U8 id) {
  GAMEPAD_STATE& s = state_[id];
  U32 lastPacketNumber = s.packetNumber;
  s = {};
  s.id = id;
  s.lastPacketNumber = lastPacketNumber;

  // call XInput GetGamepadState and convert to our GAMEPAD_STATE
  DWORD ret = xinput->GetGamepadState(id, &xinput->state_[id]);
  if (ret == ERROR_SUCCESS) {
    s.status |= GamepadStatusConnected;
    s.packetNumber = xinput->state_[id].dwPacketNumber;
    XINPUT_GAMEPAD& xiGamepad = xinput->state_[id].Gamepad;
    s.buttons = xiGamepad.wButtons;
    s.leftTrigger = xiGamepad.bLeftTrigger;
    s.rightTrigger = xiGamepad.bRightTrigger;
    s.leftThumbX = xiGamepad.sThumbLX;
    s.leftThumbY = xiGamepad.sThumbLY;
    s.rightThumbX = xiGamepad.sThumbRX;
    s.rightThumbY = xiGamepad.sThumbLY;
  } else {
    s.status |= GamepadStatusDisconnected;
  }

  return state_[id];
}

// static
bool GamepadManager::IsGamepadConnected(const GAMEPAD_STATE& state) {
  return state.status & GamepadStatusConnected;
}

}  // namespace Mana
