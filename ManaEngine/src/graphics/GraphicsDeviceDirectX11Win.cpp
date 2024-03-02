#include "pch.h"

#include "graphics/GraphicsDeviceDirectX11Win.h"

namespace Mana {

GraphicsDeviceDirectX11Win::GraphicsDeviceDirectX11Win()
    : adapter(nullptr),
      featureLevel(D3D_FEATURE_LEVEL_1_0_CORE),
      device(nullptr),
      deviceContext(nullptr) {}

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

bool GraphicsDeviceDirectX11Win::GetSupportedMultisampleLevels(
    std::vector<MultisampleLevel>& levels) {
  levels.clear();

  U32 maxSampleCount = D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT;
  U32 sampleCount = 2;

  ManaLogLnInfo(Channel::Graphics, L"Supported multisample levels:");
  U32 qualityLevels;
  while (sampleCount <= maxSampleCount) {
    HRESULT hr = device->CheckMultisampleQualityLevels(
        GraphicsDeviceDirectX11Win::dxgiFormat, sampleCount, &qualityLevels);
    if (SUCCEEDED(hr) && qualityLevels > 0) {
      MultisampleLevel level = {};
      level.SampleCount = sampleCount;
      level.QualityLevels = qualityLevels;

      ManaLogLnInfo(Channel::Graphics, L"  SampleCount: %u, QualityLevels: %u",
                    sampleCount, qualityLevels);

      levels.push_back(level);
    }

    sampleCount <<= 1;
  }

  return true;
}

}  // namespace Mana
