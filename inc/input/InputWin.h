#pragma once

#include "ManaGlobals.h"
#include "input/InputBase.h"
#include "input/RawInputWin.h"

namespace Mana {

class InputWin : public InputBase {
 public:
  InputWin(HWND hwnd);
  ~InputWin() override;

  bool Init() override;
  void Uninit() override;

  bool OnRawInput(HRAWINPUT hRawInput);

 private:
  HWND hwnd_;
  RawInputWin* pRawInput_;
};

}
