#include "pch.h"

#include "graphics/DirectX11DebugLayer.h"

#include <cassert>

// Only for debug builds
#ifdef _DEBUG

namespace Mana {

DirectX11DebugLayer::DirectX11DebugLayer() : debug_(nullptr) {}
DirectX11DebugLayer::~DirectX11DebugLayer() {
  if (debug_) {
    debug_->Release();
    debug_ = nullptr;
  }
}

bool DirectX11DebugLayer::SdkLayersAvailable() noexcept {
  HRESULT hr = D3D11CreateDevice(
      nullptr,
      D3D_DRIVER_TYPE_NULL,  // No need to create a real hardware device.
      nullptr,
      D3D11_CREATE_DEVICE_DEBUG,  // Check for SDK layers support.
      nullptr,                    // Feature level doesn't matter.
      0, D3D11_SDK_VERSION,
      nullptr,  // Don't need the D3D device.
      nullptr,  // Don't need the feature level.
      nullptr   // Don't need the D3D device context.
  );

  return SUCCEEDED(hr);
}

bool DirectX11DebugLayer::PrepareDebugInterface() {
  assert(!debug_);

  IDXGIDebug1* debug = nullptr;
  HRESULT dbgHr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug));
  if (SUCCEEDED(dbgHr)) {
    debug_ = debug;
    // Success means calling ReportLiveObjects() later on
    // will report all the live DirectX interface objects in debug builds
    // at that moment.
    return true;
  } else {
    ManaLogLnWarning(Channel::Graphics, L"DXGIGetDebugInterface1 failed");
    return false;
  }
}

void DirectX11DebugLayer::ReportLiveObjects() {
  if (!debug_) {
    return;
  }

  HRESULT hr = debug_->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
  if (FAILED(hr)) {
    ManaLogLnWarning(Channel::Graphics, L"ReportLiveObjects failed");
  }
}

}  // namespace Mana

#endif  // _DEBUG
