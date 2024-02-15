#include "pch.h"

#include "graphics/GraphicsDeviceDirectX11Win.h"

namespace Mana {

GraphicsDeviceDirectX11Win::GraphicsDeviceDirectX11Win()
    : adapter(nullptr), featureLevel(D3D_FEATURE_LEVEL_1_0_CORE) {}

GraphicsDeviceDirectX11Win::~GraphicsDeviceDirectX11Win() {
  Uninit();
}

bool GraphicsDeviceDirectX11Win::Init() {
  return true;
}

void GraphicsDeviceDirectX11Win::Uninit() {
  if (adapter) {
    adapter->Release();
    adapter = nullptr;
  }
}

}  // namespace Mana
