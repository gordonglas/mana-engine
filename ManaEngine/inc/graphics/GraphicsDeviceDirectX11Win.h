// Windows-specific Graphics Device class (DirectX11)

#pragma once

#include "graphics/DirectX11Common.h"
#include "graphics/GraphicsDeviceBase.h"

namespace Mana {


class GraphicsDeviceDirectX11Win : public GraphicsDeviceBase {
 public:
  static const DXGI_FORMAT dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

  GraphicsDeviceDirectX11Win();
  virtual ~GraphicsDeviceDirectX11Win();

  GraphicsDeviceDirectX11Win(const GraphicsDeviceDirectX11Win&) = delete;
  GraphicsDeviceDirectX11Win& operator=(const GraphicsDeviceDirectX11Win&) =
      delete;

  bool Init() override;
  void Uninit() override;

  bool GetSupportedMultisampleLevels(
      std::vector<MultisampleLevel>& levels) override;

  IDXGIAdapter1* adapter_;
  D3D_FEATURE_LEVEL featureLevel_;

  ID3D11Device3* device_;
  ID3D11DeviceContext3* deviceContext_;
};

}  // namespace Mana
