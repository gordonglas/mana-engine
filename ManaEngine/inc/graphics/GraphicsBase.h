// base Graphics Engine class

#pragma once

#include <vector>
#include "ManaGlobals.h"
#include "graphics/GraphicsDeviceBase.h"

namespace Mana {

class GraphicsBase;
extern GraphicsBase* g_pGraphicsEngine;

class GraphicsBase {
 public:
  GraphicsBase() {}
  virtual ~GraphicsBase() {}

  virtual bool Init() = 0;
  virtual void Uninit() = 0;

  virtual bool EnumerateDevices() = 0;

 protected:
  std::vector<GraphicsDeviceBase*> devices_;
};

}  // namespace Mana
