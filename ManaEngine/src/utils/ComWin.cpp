#include "pch.h"
#include "utils/Com.h"
#include <objbase.h>

namespace Mana {

int ComInitilizer::initRefCount_ = 0;

bool ComInitilizer::Init() {
  // uses COM STA
  // if (CoInitialize(nullptr) != S_OK)
  //    return false;

  // uses COM MTA
  if (CoInitializeEx(nullptr, COINIT_MULTITHREADED) != S_OK)
    return false;

  ++initRefCount_;
  return true;
}

void ComInitilizer::Uninit() {
  if (initRefCount_ > 0) {
    CoUninitialize();
    --initRefCount_;
  }
}

}  // namespace Mana
