#pragma once

#include "ManaGlobals.h"

namespace Mana {

class InputBase;
extern InputBase* g_pInputEngine;

class InputBase {
 public:
  InputBase();
  virtual ~InputBase();

  virtual bool Init() = 0;
  virtual void Uninit() = 0;

  
};

enum class InputDeviceType {
  Unknown,
  Keyboard,
  Mouse,
  Gamepad
};

constexpr U16 INPUTACTION_FLAG_E0 = 0x01;
constexpr U16 INPUTACTION_FLAG_E1 = 0x02;
constexpr U16 INPUTACTION_FLAG_RELEASE = 0x04;

// a generic way to represent key/mouse/gamepad button press/release
struct InputAction {
  // a device HANDLE (void*) on Windows (ptr addr gets converted to a U64)
  U64 deviceId;
  // if deviceType is Keyboard, the virtual key code on Windows:
  // https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
  U16 virtualKey;
  // bits:
  //  1 - set == Scan-code prefix E0
  //  2 - set == Scan-code prefix E1
  //  3 - set == key release, not set == key press
  U16 flags;
  // RAWINPUT Keyboard MakeCode on Windows (without the E0 or E1 prefix)
  // https://kbdlayout.info/kbdusx/scancodes
  U16 scanCode;
  U8 deviceType;  // InputDeviceType
};

}
