// Windows-specific Graphics Device class (DirectX11)

#pragma once

#include <dxgi1_4.h>
#include <d3d11_3.h>
#include "graphics/GraphicsDeviceBase.h"

namespace Mana {

class GraphicsDeviceDirectX11Win : public GraphicsDeviceBase {
 public:
  GraphicsDeviceDirectX11Win();
  virtual ~GraphicsDeviceDirectX11Win() override;

  bool Init() override;
  void Uninit() override;

  IDXGIAdapter1* adapter;
  D3D_FEATURE_LEVEL featureLevel;

  ID3D11Device3* device;
  ID3D11DeviceContext3* deviceContext;
};

}  // namespace Mana
