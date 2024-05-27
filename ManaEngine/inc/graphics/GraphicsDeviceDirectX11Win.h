// Windows-specific Graphics Device class (DirectX11)

#pragma once

#include "graphics/DirectX11Common.h"
#include "graphics/GraphicsDeviceBase.h"

namespace Mana {


class GraphicsDeviceDirectX11Win : public GraphicsDeviceBase {
 public:
  static const DXGI_FORMAT dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

  GraphicsDeviceDirectX11Win();
  virtual ~GraphicsDeviceDirectX11Win() override;

  bool Init() override;
  void Uninit() override;

  bool GetSupportedMultisampleLevels(
      std::vector<MultisampleLevel>& levels) override;

  IDXGIAdapter1* adapter;
  D3D_FEATURE_LEVEL featureLevel;

  ID3D11Device3* device;
  ID3D11DeviceContext3* deviceContext;
};

}  // namespace Mana
