#include "pch.h"
#include "ui/SimpleMessageBox.h"

namespace Mana {

void SimpleMessageBox::Show(const xstring& title, const xstring& message) {
  ::MessageBoxW(nullptr, message.c_str(), title.c_str(), MB_OK);
}

}  // namespace Mana
