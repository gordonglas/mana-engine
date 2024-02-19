// Windows-specific Graphics Engine functionality (DirectX 11)

#pragma once

#include <vector>
#include "graphics/GraphicsBase.h"
#include "graphics/GraphicsDeviceDirectX11Win.h"
#include "target/TargetOS.h"

// dxgi1_4 was introduced in Windows 8.1
// dxgi1_5 was introduced in Windows 10, version 1903 (10.0; Build 18362)
// Since we're supporting Win10+, we'll use dxgi1_4.
// dxgi1_4 adds the newer flip swap chain modes for better windowed-mode performance.
// See: https://walbourn.github.io/care-and-feeding-of-modern-swapchains/
//      https://stackoverflow.com/questions/42354369/idxgifactory-versions
#include <dxgi1_4.h>

// Require DirectX 11.3 (Requires Windows 10+)
// See: https://walbourn.github.io/anatomy-of-direct3d-11-create-device/
#include <d3d11_3.h>

namespace Mana {

class GraphicsDirectX11Win : public GraphicsBase {
 public:
  bool Init() override;
  void Uninit() override;

  bool EnumerateAdaptersAndFullScreenModes() override;
  xstring GetNoSupportedGPUFoundMessage() override;

  bool GetSupportedGPUs(std::vector<GraphicsDeviceBase*>& gpus) override;

  bool SelectGPU(GraphicsDeviceBase* gpu) override;
};

}  // namespace Mana
