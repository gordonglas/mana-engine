#include "pch.h"
#include "graphics/GraphicsDirectX11Win.h"

#include <dxgi.h>
#pragma comment(lib, "dxgi.lib")

//#include <D3D11.h>
//#pragma comment(lib, "d3d11.lib")

#include <vector>

namespace Mana {

GraphicsBase* g_pGraphicsEngine = nullptr;

bool GraphicsDirectX11Win::Init() {
  // TODO: maybe move ManaGame's initial call to EnumerateDevices here?
  return true;
}

void GraphicsDirectX11Win::Uninit() {
}

bool GraphicsDirectX11Win::EnumerateDevices() {
  IDXGIFactory1* pFactory = nullptr;
  if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&pFactory))) {
    return false;
  }

  UINT i = 0;
  IDXGIAdapter1* pAdapter;
  std::vector<IDXGIAdapter1*> vAdapters;
  while (pFactory->EnumAdapters1(i, &pAdapter) != DXGI_ERROR_NOT_FOUND) {
    vAdapters.push_back(pAdapter);
    ++i;
  }

  // Order of adapters:
  // - Adapter with the output on which the desktop primary is displayed.
  //   This adapter corresponds with an index of zero.
  // - Adapters with outputs.
  // - Adapters without outputs.

  // TODO: Use vAdapters before function exits.
  //       Need to determine what this function will save.
  //       (adaptors + devices?)

  for (IDXGIAdapter1* adapter : vAdapters) {
    DXGI_ADAPTER_DESC desc;
    if (FAILED(adapter->GetDesc(&desc))) {
      continue;
    }

    // TODO: determine if we want to keep this adapter or not.
    //       at least, it must have an output.
    xstring adapterDesc(desc.Description);
    OutputDebugStringW(L"Adaptor Description: ");
    OutputDebugStringW(adapterDesc.c_str());

    //devices_.push_back((GraphicsDeviceBase*)new GraphicsDeviceDirectX11Win());
  }

  for (IDXGIAdapter1* adapter : vAdapters) {
    adapter->Release();
  }

  // When you create a factory, the factory enumerates the set
  // of adapters that are available in the system. Therefore,
  // if you change the adapters in a system, you must destroy
  // and recreate the IDXGIFactory1 object. The number of adapters
  // in a system changes when you add or remove a display card,
  // or dock or undock a laptop.
  if (pFactory) {
    pFactory->Release();
  }

  return true;
}

}  // namespace Mana
