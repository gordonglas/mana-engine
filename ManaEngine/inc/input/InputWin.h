#pragma once

#include "ManaGlobals.h"
#include "input/InputBase.h"
#include "input/RawInputWin.h"

namespace Mana {

class InputWin : public InputBase {
 public:
  InputWin(HWND hwnd);
  virtual ~InputWin() = default;

  bool Init() override;
  void Uninit() override;

  bool OnRawInput(HRAWINPUT hRawInput);
  bool OnInputDeviceChange(InputDeviceChangeType deviceChangeType,
                           U64 deviceId) override;
  void OnMouseMove(WPARAM wParam, LPARAM lParam);

 private:
  HWND hwnd_;
  RawInputWin* pRawInput_;
};

}  // namespace Mana
