// base Graphics Device class

#pragma once

namespace Mana {

class GraphicsDeviceBase {
 public:
  GraphicsDeviceBase() {}
  virtual ~GraphicsDeviceBase() {}

  virtual bool Init() = 0;
  virtual void Uninit() = 0;

  xstring name;
};

}  // namespace Mana
