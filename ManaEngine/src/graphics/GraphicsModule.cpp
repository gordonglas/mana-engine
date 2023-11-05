#include "pch.h"

#include "graphics/GraphicsModule.h"

namespace Mana {

GraphicsModule::GraphicsModule() = default;

GraphicsModule::~GraphicsModule() {
  module_.Unload();
}

bool GraphicsModule::Load() {
  if (!module_.Load(_X("ManaDirectX.dll"))) {
    return false;
  }

  // TODO: setup exported DLL function pointers

  return true;
}

bool GraphicsModule::Unload() {
  return module_.Unload();
}

const xstring& GraphicsModule::ModuleName() const {
  return module_.Name();
}

}
