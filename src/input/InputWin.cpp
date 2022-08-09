#include "pch.h"
#include "input/InputWin.h"

namespace Mana {

InputBase* g_pInputEngine = nullptr;

InputWin::InputWin() {}
InputWin::~InputWin() {}

bool InputWin::Init() {
  return true;
}

void InputWin::Uninit() {

}

}  // namespace Mana
