#include "pch.h"
#include "utils/ScopedComInitializer.h"
#include <objbase.h>

namespace Mana {

ScopedComInitializer::ScopedComInitializer() {
  // Uses COM STA
  //HRESULT hr =
  //    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
  //initialized_ = SUCCEEDED(hr);  // S_OK and S_FALSE evaluate to true

  // Uses COM MTA
  // See: https://learn.microsoft.com/en-us/windows/win32/learnwin32/initializing-the-com-library
  HRESULT hr =
      CoInitializeEx(nullptr, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);
  initialized_ = SUCCEEDED(hr); // S_OK and S_FALSE evaluate to true
}

ScopedComInitializer::~ScopedComInitializer() {
  if (initialized_) {
    CoUninitialize();
  }
}

}  // namespace Mana
