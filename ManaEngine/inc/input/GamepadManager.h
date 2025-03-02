#pragma once

#include "ManaGlobals.h"

namespace Mana {

enum class GamepadType {
  Unknown,
  Xbox, // AKA XInput
  //DualShock4,
  //DualSense,
  //LeftJoyCon,
  //RightJoyCon,
  //SwitchPro,
};

constexpr U8 GamepadStatusDisconnected   = 0x00;
constexpr U8 GamepadStatusConnected      = 0x01;

struct GAMEPAD_STATE {
  U8 id;             // 0 to maxGamepads_ - 1
  U8 type;           // GamepadType enum
  // bitfield:
  //  bit 0 = GamepadStatusDisconnected
  //  bit 1 = GamepadStatusConnected
  //  bits 0 and 1 are mutually exclusive
  U8 status;
  U32 lastPacketNumber;
  U32 packetNumber;  // if changed since last GAMEPAD_STATE (per gamepad),
                     // GAMEPAD_STATE has been updated.
  // https://learn.microsoft.com/en-us/windows/win32/api/xinput/ns-xinput-xinput_gamepad
  U16 buttons;
  U8 leftTrigger;
  U8 rightTrigger;
  I16 leftThumbX;
  I16 leftThumbY;
  I16 rightThumbX;
  I16 rightThumbY;
};

class GamepadManager {
 public:
  GamepadManager();
  virtual ~GamepadManager() = default;

  GamepadManager(const GamepadManager&) = delete;
  GamepadManager& operator=(const GamepadManager&) = delete;

  bool Init();
  void Uninit();

  GAMEPAD_STATE& GetGamepadState(U8 id);
  
  static bool IsGamepadConnected(const GAMEPAD_STATE& state);

 private:
  GAMEPAD_STATE* state_;
};

}  // namespace Mana
