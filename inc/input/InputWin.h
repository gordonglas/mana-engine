#pragma once

#include "input/InputBase.h"

namespace Mana {

class InputWin : public InputBase {
 public:
  InputWin();
  ~InputWin() override;

  bool Init() override;
  void Uninit() override;


};

}
