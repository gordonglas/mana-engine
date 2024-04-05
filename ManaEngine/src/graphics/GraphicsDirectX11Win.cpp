#include "pch.h"
#include "graphics/GraphicsDirectX11Win.h"

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxguid.lib")

// ComPtr<T> - See: https://github.com/Microsoft/DirectXTK/wiki/ComPtr
#include <wrl/client.h>
#include <vector>

// TODO: Correctly handle lifetimes of all IDXGI interface pointers.
//       Use ComPtr<T>

namespace {

#ifdef _DEBUG
// Checks for SDK Layers support (AKA, the DirectX Debug layer)
// https://learn.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-devices-layers#debug-layer
// For Windows 10, to create a device that supports the debug layer,
// enable the "Graphics Tools" optional feature. Go to the Settings panel,
// under System, Apps & features, Manage optional Features, Add a feature,
// and then look for "Graphics Tools".
bool SdkLayersAvailable() noexcept {
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
#endif  // DEBUG

}  // namespace

namespace Mana {

GraphicsBase* g_pGraphicsEngine = nullptr;

GraphicsDirectX11Win::GraphicsDirectX11Win()
#ifdef _DEBUG
    : debug_(nullptr)
#endif
{
}

bool GraphicsDirectX11Win::Init() {
  return true;
}

void GraphicsDirectX11Win::Uninit() {
}

bool GraphicsDirectX11Win::EnumerateAdaptersAndFullScreenModes() {
  IDXGIFactory1* pFactory1 = nullptr;
  if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&pFactory1)))) {
    ManaLogLnError(
        Channel::Graphics,
        L"EnumerateAdaptersAndFullScreenModes: CreateDXGIFactory1 failed");
    return false;
  }

  IDXGIFactory4* pFactory4 = nullptr;
  if (FAILED(pFactory1->QueryInterface(IID_PPV_ARGS(&pFactory4)))) {
    ManaLogLnError(Channel::Graphics,
                   L"EnumerateAdaptersAndFullScreenModes: Failed to "
                   L"QueryInterface IDXGIFactory4");
    pFactory1->Release();
    return false;
  }

  UINT i = 0;
  IDXGIAdapter1* pAdapter;
  std::vector<IDXGIAdapter1*> adapters;
  while (pFactory4->EnumAdapters1(i, &pAdapter) != DXGI_ERROR_NOT_FOUND) {
    adapters.push_back(pAdapter);
    ++i;
  }

  // Order of adapters:
  // - Adapter with the output on which the desktop primary is displayed.
  //   This adapter corresponds with an index of zero.
  // - Adapters with outputs.
  // - Adapters without outputs.

  for (IDXGIAdapter1* adapter : adapters) {
    DXGI_ADAPTER_DESC1 adapterDesc;
    if (FAILED(adapter->GetDesc1(&adapterDesc))) {
      continue;
    }

    ManaLogLnInfo(Channel::Graphics, L"Adaptor: %s",
                  adapterDesc.Description);
    ManaLogLnInfo(Channel::Graphics, L"  Dedicated video memory: %llu",
                  adapterDesc.DedicatedVideoMemory);
    ManaLogLnInfo(Channel::Graphics, L"  Dedicated system memory: %llu",
                  adapterDesc.DedicatedSystemMemory);
    ManaLogLnInfo(Channel::Graphics, L"  Shared system memory: %llu",
                  adapterDesc.SharedSystemMemory);

    bool isSoftwareRenderer = adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE;
    ManaLogLnInfo(Channel::Graphics, L"  Is software renderer: %s",
                  isSoftwareRenderer ? L"true" : L"false");

    // Info about "Microsoft Basic Render Driver":
    // https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/d3d10-graphics-programming-guide-dxgi#new-info-about-enumerating-adapters-for-windows-8

    i = 0;
    IDXGIOutput* pOutput;
    std::vector<IDXGIOutput*> outputs;
    while (adapter->EnumOutputs(i, &pOutput) != DXGI_ERROR_NOT_FOUND) {
      outputs.push_back(pOutput);
      ++i;
    }

    ManaLogLnInfo(Channel::Graphics, L"  Num outputs: %llu", outputs.size());

    // EnumOutputs first returns the output on which the desktop primary
    // is displayed. This output corresponds with an index of zero.

    for (IDXGIOutput* output : outputs) {
      DXGI_OUTPUT_DESC outputDesc;
      if (FAILED(output->GetDesc(&outputDesc))) {
        continue;
      }

      ManaLogLnInfo(Channel::Graphics, L"  Output name: %s",
                    outputDesc.DeviceName);
      ManaLogLnInfo(Channel::Graphics, L"    DesktopCoordinates:");
      ManaLogLnInfo(Channel::Graphics, L"      Left: %ld",
                    outputDesc.DesktopCoordinates.left);
      ManaLogLnInfo(Channel::Graphics, L"      Top: %ld",
                    outputDesc.DesktopCoordinates.top);
      ManaLogLnInfo(Channel::Graphics, L"      Right: %ld",
                    outputDesc.DesktopCoordinates.right);
      ManaLogLnInfo(Channel::Graphics, L"      Bottom: %ld",
                    outputDesc.DesktopCoordinates.bottom);
      ManaLogLnInfo(Channel::Graphics, L"    AttachedToDesktop: %s",
                    outputDesc.AttachedToDesktop ? L"true" : L"false");

      xstring rotation(L"UNHANDLED");
      switch (outputDesc.Rotation) {
        case DXGI_MODE_ROTATION_UNSPECIFIED:
          rotation = L"Unspecified";
          break;
        case DXGI_MODE_ROTATION_IDENTITY:
          rotation = L"Identity";
          break;
        case DXGI_MODE_ROTATION_ROTATE90:
          rotation = L"Rotate90";
          break;
        case DXGI_MODE_ROTATION_ROTATE180:
          rotation = L"Rotate180";
          break;
        case DXGI_MODE_ROTATION_ROTATE270:
          rotation = L"Rotate270";
          break;
      }
      ManaLogLnInfo(Channel::Graphics, L"    Rotation: %s", rotation.c_str());

      // Get display modes for the format this engine supports,
      // which is guaranteed to be supported by the Direct11 hardware.
      // See: https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/format-support-for-direct3d-11-0-feature-level-hardware#dxgi_format_r8g8b8a8_unormfcs-28

      // Get number of modes.
      ManaLogLnInfo(Channel::Graphics,
                  L"    Output modes for format DXGI_FORMAT_R8G8B8A8_UNORM:");
      unsigned numModes = 0;
      if (FAILED(output->GetDisplayModeList(
              GraphicsDeviceDirectX11Win::dxgiFormat, 0, &numModes, nullptr))) {
        continue;
      }

      // We don't care about this adapter if it doesn't have any outputs.
      if (numModes == 0) {
        ManaLogLnInfo(Channel::Graphics, L"    Num output modes: %u", numModes);
        continue;
      }

      // Get modes.
      DXGI_MODE_DESC* pModes = new DXGI_MODE_DESC[numModes];
      if (FAILED(output->GetDisplayModeList(
              GraphicsDeviceDirectX11Win::dxgiFormat, 0, &numModes, pModes))) {
        continue;
      }

      for (unsigned m = 0; m < numModes; ++m) {
        DXGI_MODE_DESC& mode = pModes[m];
        ManaLogLnInfo(Channel::Graphics, L"    Mode: %d", m + 1);
        ManaLogLnInfo(Channel::Graphics, L"      Width: %u", mode.Width);
        ManaLogLnInfo(Channel::Graphics, L"      Height: %u", mode.Height);
        ManaLogLnInfo(Channel::Graphics, L"      RefreshRate: %u / %u",
                      mode.RefreshRate.Numerator, mode.RefreshRate.Denominator);

        xstring scanlineOrder(L"UNHANDLED");
        switch (mode.ScanlineOrdering) {
          case DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED:
            scanlineOrder = L"Unspecified";
            break;
          case DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE:
            scanlineOrder = L"Progressive";
            break;
          case DXGI_MODE_SCANLINE_ORDER_UPPER_FIELD_FIRST:
            scanlineOrder = L"Upper field first";
            break;
          case DXGI_MODE_SCANLINE_ORDER_LOWER_FIELD_FIRST:
            scanlineOrder = L"Lower field first";
            break;
        }
        ManaLogLnInfo(Channel::Graphics, L"      Scanline order: %s",
                      scanlineOrder.c_str());

        xstring scaling(L"UNHANDLED");
        switch (mode.Scaling) {
          case DXGI_MODE_SCALING_UNSPECIFIED:
            scaling = L"Unspecified";
            break;
          case DXGI_MODE_SCALING_CENTERED:
            scaling = L"Centered";
            break;
          case DXGI_MODE_SCALING_STRETCHED:
            scaling = L"Stretched";
            break;
        }
        ManaLogLnInfo(Channel::Graphics, L"      Scaling: %s", scaling.c_str());
      }

      // TODO: store data into cross-platform objects? See GraphicsBase.h

      delete[] pModes;
      pModes = nullptr;
    }

    for (IDXGIOutput* output : outputs) {
      output->Release();
    }
  }

  for (IDXGIAdapter1* adapter : adapters) {
    adapter->Release();
  }

  // When you create a factory, the factory enumerates the set
  // of adapters that are available in the system. Therefore,
  // if you change the adapters in a system, you must destroy
  // and recreate the IDXGIFactory object. The number of adapters
  // in a system changes when you add or remove a display card,
  // or dock or undock a laptop.
  if (pFactory4) {
    pFactory4->Release();
  }
  if (pFactory1) {
    pFactory1->Release();
  }

  return true;
}

xstring GraphicsDirectX11Win::GetNoSupportedGPUFoundMessage() {
  return L"Sorry...\nA DirectX 11 GPU is required to play this game.";
}

bool GraphicsDirectX11Win::GetSupportedGPUs(
    std::vector<GraphicsDeviceBase*>& gpus) {
  gpus.clear();

  D3D_FEATURE_LEVEL featureLevels[] = {D3D_FEATURE_LEVEL_11_1,
                                       D3D_FEATURE_LEVEL_11_0};

  // Don't just try the default GPU... loop through EnumAdapters,
  // only allowing devices with an output and filtering out software devices

  IDXGIFactory1* pFactory1 = nullptr;
  if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&pFactory1)))) {
    ManaLogLnError(Channel::Graphics,
                   L"GetSupportedGPUs: CreateDXGIFactory1 failed");
    return false;
  }

  IDXGIFactory4* pFactory4 = nullptr;
  if (FAILED(pFactory1->QueryInterface(IID_PPV_ARGS(&pFactory4)))) {
    ManaLogLnError(Channel::Graphics,
                   L"GetSupportedGPUs: Failed to QueryInterface IDXGIFactory4");
    pFactory1->Release();
    return false;
  }

  UINT i = 0;
  IDXGIAdapter1* pAdapter;
  std::vector<IDXGIAdapter1*> adapters;
  while (pFactory4->EnumAdapters1(i, &pAdapter) != DXGI_ERROR_NOT_FOUND) {
    adapters.push_back(pAdapter);
    ++i;
  }

  // Order of adapters:
  // - Adapter with the output on which the desktop primary is displayed.
  //   This adapter corresponds with an index of zero.
  // - Adapters with outputs.
  // - Adapters without outputs.
  int j = -1;
  for (IDXGIAdapter1* adapter : adapters) {
    ++j;
    DXGI_ADAPTER_DESC1 adapterDesc;
    if (FAILED(adapter->GetDesc1(&adapterDesc))) {
      continue;
    }

    // Skip the "Microsoft Basic Render Driver"
    if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
      continue;
    }

    i = 0;
    IDXGIOutput* pOutput;
    std::vector<IDXGIOutput*> outputs;
    while (adapter->EnumOutputs(i, &pOutput) != DXGI_ERROR_NOT_FOUND) {
      outputs.push_back(pOutput);
      ++i;
    }

    // Must have at least one output
    if (outputs.size() == 0) {
      continue;
    }

    for (IDXGIOutput* output : outputs) {
      output->Release();
    }

    // If you set the pAdapter parameter to a non-NULL value,
    // you must also set the DriverType parameter to the
    // D3D_DRIVER_TYPE_UNKNOWN value.

    // Set ppDevice and ppImmediateContext to NULL to determine which feature
    // level is supported by looking at pFeatureLevel without creating a device.
    D3D_FEATURE_LEVEL supportedFeatureLevel;
    HRESULT hr = D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0,
                                   featureLevels, _countof(featureLevels),
                                   D3D11_SDK_VERSION,
                                   /*ppDevice=*/nullptr, &supportedFeatureLevel,
                                   /*ppImmediateContext=*/nullptr);

    // If you provide a D3D_FEATURE_LEVEL array that contains
    // D3D_FEATURE_LEVEL_11_1 on a computer that doesn't have
    // the Direct3D 11.1 runtime installed, this function immediately fails with
    // E_INVALIDARG...
    if (hr == E_INVALIDARG) {
      // ...so try D3D_FEATURE_LEVEL_11_0 next
      hr = D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0,
                             &featureLevels[1], _countof(featureLevels) - 1,
                             D3D11_SDK_VERSION,
                             /*ppDevice=*/nullptr, &supportedFeatureLevel,
                             /*ppImmediateContext=*/nullptr);
      if (FAILED(hr)) {
        ManaLogLnError(Channel::Graphics,
                       L"D3D11CreateDevice failed: HRESULT: %ld", hr);
        continue;
      }
    }

    GraphicsDeviceDirectX11Win* gpu = new GraphicsDeviceDirectX11Win();
    gpu->name = adapterDesc.Description;
    gpu->adapter = adapter;
    gpu->adapter->AddRef();
    gpu->featureLevel = supportedFeatureLevel;

    gpus.push_back(gpu);

    ManaLogLnInfo(Channel::Graphics, L"DX11 GPU found (index %d): %s", j,
                  adapterDesc.Description);
    xstring featureLevel(L"UNHANDLED");
    switch (gpu->featureLevel) {
      case D3D_FEATURE_LEVEL_11_1:
        featureLevel = L"D3D_FEATURE_LEVEL_11_1";
        break;
      case D3D_FEATURE_LEVEL_11_0:
        featureLevel = L"D3D_FEATURE_LEVEL_11_0";
        break;
    }
    ManaLogLnInfo(Channel::Graphics, L"  Feature level: %s",
                  featureLevel.c_str());
  }

  for (IDXGIAdapter1* adapter : adapters) {
    adapter->Release();
  }

  if (pFactory4) {
    pFactory4->Release();
  }
  if (pFactory1) {
    pFactory1->Release();
  }

  return true;
}

// TODO: handle cases when calling SelectGPU multiple times - Release/recreate objects.
bool GraphicsDirectX11Win::SelectGPU(GraphicsDeviceBase* gpuBase) {
  GraphicsDeviceDirectX11Win* gpu = (GraphicsDeviceDirectX11Win*)gpuBase;

  D3D_FEATURE_LEVEL featureLevel[1] = {gpu->featureLevel};

  U32 creationFlags = 0;
#ifdef _DEBUG
  if (SdkLayersAvailable()) {
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
  } else {
    ManaLogLnWarning(Channel::Graphics, L"D3D11 Debug Device is not available");
  }
#endif

  ID3D11Device* pDevice;
  ID3D11DeviceContext* pDeviceContext;
  D3D_FEATURE_LEVEL supportedFeatureLevel;
  HRESULT hr =
      D3D11CreateDevice(gpu->adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr,
                        creationFlags, featureLevel, 1, D3D11_SDK_VERSION,
                        &pDevice, &supportedFeatureLevel, &pDeviceContext);
  if (FAILED(hr)) {
    ManaLogLnInfo(Channel::Graphics, L"SelectGPU D3D11CreateDevice failed");
    return false;
  }

#ifdef _DEBUG
  // if using debug layer, get debug interface so we can call ReportLiveObjects
  if (creationFlags & D3D11_CREATE_DEVICE_DEBUG) {
    IDXGIDebug1* debug = nullptr;
    HRESULT dbgHr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug));
    if (SUCCEEDED(dbgHr)) {
      debug_ = debug;
      // Success means calling this->Debug_ReportLiveObjects() later on
      // will report all the live DirectX interface objects in debug builds
      // at that moment.
    } else {
      ManaLogLnWarning(Channel::Graphics, L"DXGIGetDebugInterface1 failed");
    }
  }
#endif  // DEBUG

  // update interfaces to Direct3D 11.3
  // https://github.com/walbourn/directx-sdk-samples/blob/main/DXUT/Core/DXUT.cpp#L2634
  ID3D11Device3* pDevice3 = nullptr;
  hr = pDevice->QueryInterface(IID_PPV_ARGS(&pDevice3));
  if (FAILED(hr) || !pDevice3) {
    ManaLogLnInfo(Channel::Graphics,
                  L"SelectGPU QueryInterface ID3D11Device3 failed");
    return false;
  }

  gpu->device = pDevice3;

  ID3D11DeviceContext3* pDeviceContext3 = nullptr;
  hr = pDeviceContext->QueryInterface(IID_PPV_ARGS(&pDeviceContext3));
  if (FAILED(hr) || !pDeviceContext3) {
    ManaLogLnInfo(Channel::Graphics,
                  L"SelectGPU QueryInterface ID3D11DeviceContext3 failed");
    return false;
  }

  gpu->deviceContext = pDeviceContext3;

  return true;
}

#ifdef _DEBUG
void GraphicsDirectX11Win::Debug_ReportLiveObjects() {
  if (!debug_) {
    return;
  }

  HRESULT hr = debug_->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
  if (FAILED(hr)) {
    ManaLogLnWarning(Channel::Graphics, L"ReportLiveObjects failed");
  }
}
#endif

}  // namespace Mana
