// Windows-specific Graphics Engine functionality (DirectX 11)

#pragma once

#include <vector>
#include "graphics/DirectX11Common.h"
#include "graphics/GraphicsBase.h"
#include "graphics/GraphicsDeviceDirectX11Win.h"

#ifdef _DEBUG
// A macro to make it easier to call ReportLiveObjects
#define DXDBG_REPORT_LIVE_OBJECTS(dx11) \
  do {                                     \
    dx11->ReportLiveObjects();             \
  } while (0)
#else
#define DXDBG_REPORT_LIVE_OBJECTS(dx11)
#endif

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
  void ReportLiveObjects();
#endif

 private:
#ifdef _DEBUG
  DirectX11DebugLayer* debug_;
#endif
};

}  // namespace Mana
