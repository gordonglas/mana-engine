// Windows-specific Graphics Device class (DirectX11)

#pragma once

#include "graphics/GraphicsDeviceBase.h"

namespace Mana {

class GraphicsDeviceDirectX11Win : GraphicsDeviceBase {
 public:
  bool Init() override;
  void Uninit() override;
};

}  // namespace Mana
