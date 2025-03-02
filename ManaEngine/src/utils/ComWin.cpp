#include "pch.h"
#include "utils/Com.h"
#include <objbase.h>

namespace Mana {

// static
int ComInitializer::initRefCount_ = 0;

// static
bool ComInitializer::Init() {
  // uses COM STA
  // if (CoInitialize(nullptr) != S_OK)
  //    return false;

  // uses COM MTA
  if (CoInitializeEx(nullptr, COINIT_MULTITHREADED) != S_OK)
    return false;

  ++initRefCount_;
  return true;
}

// static
void ComInitializer::Uninit() {
  if (initRefCount_ > 0) {
    CoUninitialize();
    --initRefCount_;
  }
}

}  // namespace Mana
