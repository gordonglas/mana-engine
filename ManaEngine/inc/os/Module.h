#pragma once

#include "ManaGlobals.h"
#include "target/TargetOS.h"

namespace Mana {

// Loads/unloads a program module (dll, etc)
class Module {
 public:
  Module();
  ~Module();

  bool Load(xstring name);
  bool Unload();

  const xstring& Name() const { return name_; }

#ifdef OS_WIN
  HMODULE GetHandle() { return hModule_; }
#endif

 private:
  xstring name_;
#ifdef OS_WIN
  HMODULE hModule_;
#endif
};

}  // namespace Mana
