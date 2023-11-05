#pragma once

#include "ManaGlobals.h"
#include "os/Module.h"

namespace Mana {

class GraphicsModule {
 public:
  GraphicsModule();
  ~GraphicsModule();

  // TODO: pass in graphics config
  bool Load();
  bool Unload();

  const xstring& ModuleName() const;

 private:
  Module module_;
};

}  // namespace Mana
