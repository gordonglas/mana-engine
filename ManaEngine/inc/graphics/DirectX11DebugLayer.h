// DirectX 11 Debug Layer functionality

#pragma once

#include "graphics/DirectX11Common.h"

// Only for debug builds
#ifdef _DEBUG

#include <dxgidebug.h>

namespace Mana {

class DirectX11DebugLayer {
 public:
  DirectX11DebugLayer();
  ~DirectX11DebugLayer();

  DirectX11DebugLayer(const DirectX11DebugLayer&) = delete;
  DirectX11DebugLayer& operator=(const DirectX11DebugLayer&) = delete;

  // Checks for SDK Layers support (AKA, the DirectX Debug layer)
  // https://learn.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-devices-layers#debug-layer
  // For Windows 10, to create a device that supports the debug layer,
  // enable the "Graphics Tools" optional feature. Go to the Settings panel,
  // under System, Apps & features, Manage optional Features, Add a feature,
  // and then look for "Graphics Tools".
  bool SdkLayersAvailable() noexcept;

  // If returns true, we can then call ReportLiveObjects().
  bool PrepareDebugInterface();

  // Prints live DirectX interface object info to the IDE Output window.
  // See https://web.archive.org/web/20170606112607/http://seanmiddleditch.com/direct3d-11-debug-api-tricks/
  void ReportLiveObjects();

 private:
  IDXGIDebug1* debug_;
};

}  // namespace Mana

#endif  // _DEBUG
