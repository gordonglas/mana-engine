// Windows-specific Graphics Engine functionality (DirectX 11)

#pragma once

#include <vector>
#include "graphics/DirectX11Common.h"
// TODO: HERE!!! move debug layer stuff into....
#include "graphics/DirectX11DebugLayer.h"
#include "graphics/GraphicsBase.h"
#include "graphics/GraphicsDeviceDirectX11Win.h"

namespace Mana {

class GraphicsDirectX11Win : public GraphicsBase {
 public:
  GraphicsDirectX11Win();

  bool Init() override;
  void Uninit() override;

  bool EnumerateAdaptersAndFullScreenModes() override;
  xstring GetNoSupportedGPUFoundMessage() override;

  bool GetSupportedGPUs(std::vector<GraphicsDeviceBase*>& gpus) override;

  bool SelectGPU(GraphicsDeviceBase* gpu) override;

#ifdef _DEBUG
  // Prints live DirectX interface object info to the IDE Output window.
  // See https://web.archive.org/web/20170606112607/http://seanmiddleditch.com/direct3d-11-debug-api-tricks/
  void Debug_ReportLiveObjects();
#endif

#ifdef _DEBUG
  IDXGIDebug1* debug_;
#endif
};

}  // namespace Mana
