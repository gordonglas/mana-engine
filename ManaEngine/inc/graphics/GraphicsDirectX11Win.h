// Windows-specific Graphics Engine functionality (DirectX 11)

#pragma once

#include "graphics/GraphicsBase.h"
#include "graphics/GraphicsDeviceDirectX11Win.h"
#include "target/TargetOS.h"

namespace Mana {

class GraphicsDirectX11Win : public GraphicsBase {
 public:
  bool Init() override;
  void Uninit() override;

  // Refreshes list of video devices that have outputs
  bool EnumerateDevices() override;
};

}  // namespace Mana
