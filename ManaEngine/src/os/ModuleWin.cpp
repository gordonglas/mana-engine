#include "pch.h"

#include <cassert>
#include "os/Module.h"

namespace Mana {

Module::Module() : hModule_(nullptr) {}

Module::~Module() {
  Unload();
}

bool Module::Load(xstring name) {
  assert(!hModule_);

  name_ = name;
  hModule_ = ::LoadLibraryW(name_.c_str());
  if (!hModule_) {
    return false;
  }

  return true;
}

bool Module::Unload() {
  if (!hModule_) {
    return false;
  }

  BOOL ret = ::FreeLibrary(hModule_);
  hModule_ = nullptr;
  name_.clear();
  return ret == TRUE;
}

}  // namespace Mana
