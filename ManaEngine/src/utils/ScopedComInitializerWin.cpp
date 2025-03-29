#include "pch.h"
#include "utils/ScopedComInitializer.h"

#include <cassert>
#include <objbase.h>

namespace Mana {

ScopedComInitializer::~ScopedComInitializer() {
  if (initialized_) {
    CoUninitialize();
  }
}

bool ScopedComInitializer::Init() {
  assert(!initialized_);

  // Uses COM STA
  // HRESULT hr =
  //     CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED |
  //     COINIT_DISABLE_OLE1DDE);

  // Uses COM MTA
  // See:
  // https://learn.microsoft.com/en-us/windows/win32/learnwin32/initializing-the-com-library
  HRESULT hr =
      CoInitializeEx(nullptr, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);

  initialized_ = SUCCEEDED(hr);  // S_OK and S_FALSE evaluate to true
  return initialized_;
}

}  // namespace Mana
