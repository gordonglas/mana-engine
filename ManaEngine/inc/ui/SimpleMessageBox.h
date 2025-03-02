#pragma once

#include "ManaGlobals.h"

namespace Mana {

// cross-platform simplified MessageBox
class SimpleMessageBox {
 public:
  virtual ~SimpleMessageBox() = default;

  static void Show(const xstring& title, const xstring& message);
};

}  // namespace Mana
