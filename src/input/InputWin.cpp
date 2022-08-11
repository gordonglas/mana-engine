#include "pch.h"
#include "input/InputWin.h"

namespace Mana {

InputBase* g_pInputEngine = nullptr;

InputWin::InputWin(HWND hwnd) : hwnd_(hwnd), pRawInput_(nullptr) {}
InputWin::~InputWin() {}

bool InputWin::Init() {
  pRawInput_ = new RawInputWin(hwnd_);
  if (!pRawInput_)
    return false;
  if (!pRawInput_->Init())
    return false;
  return true;
}

void InputWin::Uninit() {
  if (pRawInput_) {
    pRawInput_->Uninit();
    delete pRawInput_;
    pRawInput_ = nullptr;
  }
}

bool InputWin::OnRawInput(HRAWINPUT hRawInput) {
  return pRawInput_->OnRawInput(hRawInput);
}

}  // namespace Mana
