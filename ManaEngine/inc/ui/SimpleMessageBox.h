#pragma once

#include "ManaGlobals.h"

namespace Mana {

// cross-platform simplified MessageBox
class SimpleMessageBox {
 public:
  static void Show(const xstring& title, const xstring& message);
};

}  // namespace Mana
