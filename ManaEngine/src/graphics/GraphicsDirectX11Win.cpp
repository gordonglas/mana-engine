#include "pch.h"
#include "graphics/GraphicsDirectX11Win.h"

#include <dxgi.h>
#pragma comment(lib, "dxgi.lib")

//#include <D3D11.h>
//#pragma comment(lib, "d3d11.lib")

#include <vector>

namespace Mana {

GraphicsBase* g_pGraphicsEngine = nullptr;
DXGI_FORMAT dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

bool GraphicsDirectX11Win::Init() {
  // TODO: Maybe move ManaGame's initial call to
  //       EnumerateAdaptersAndFullScreenModes here?
  return true;
}

void GraphicsDirectX11Win::Uninit() {
}

bool GraphicsDirectX11Win::EnumerateAdaptersAndFullScreenModes() {
  IDXGIFactory1* pFactory = nullptr;
  if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&pFactory))) {
    return false;
  }

  UINT i = 0;
  IDXGIAdapter1* pAdapter;
  std::vector<IDXGIAdapter1*> adapters;
  while (pFactory->EnumAdapters1(i, &pAdapter) != DXGI_ERROR_NOT_FOUND) {
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

    // TODO: determine if we want to keep this adapter or not.
    //       at least, it must have an output.

    ManaLogLnInfo(Channel::Graphics, L"Adaptor: %s",
                  adapterDesc.Description);
    ManaLogLnInfo(Channel::Graphics, L"  Dedicated video memory: %llu",
                  adapterDesc.DedicatedVideoMemory);
    ManaLogLnInfo(Channel::Graphics, L"  Dedicated system memory: %llu",
                  adapterDesc.DedicatedSystemMemory);
    ManaLogLnInfo(Channel::Graphics, L"  Shared system memory: %llu",
                  adapterDesc.SharedSystemMemory);

    bool hasSoftwareRenderer = adapterDesc.Flags | DXGI_ADAPTER_FLAG_SOFTWARE;
    ManaLogLnInfo(Channel::Graphics, L"  Has software renderer: %s",
                  hasSoftwareRenderer ? L"true" : L"false");

    // Info about "Microsoft Basic Render Driver":
    // https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/d3d10-graphics-programming-guide-dxgi#new-info-about-enumerating-adapters-for-windows-8

    i = 0;
    IDXGIOutput* pOutput;
    std::vector<IDXGIOutput*> outputs;
    while (adapter->EnumOutputs(i, &pOutput) != DXGI_ERROR_NOT_FOUND) {
      outputs.push_back(pOutput);
      ++i;
    }

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
      if (FAILED(
              output->GetDisplayModeList(dxgiFormat, 0, &numModes, nullptr))) {
        continue;
      }

      // We don't care about this adapter if it doesn't have any outputs.
      if (numModes == 0) {
        ManaLogLnInfo(Channel::Graphics, L"    Num output modes: %u", numModes);
        continue;
      }

      // Get modes.
      DXGI_MODE_DESC* pModes = new DXGI_MODE_DESC[numModes];
      if (FAILED(
              output->GetDisplayModeList(dxgiFormat, 0, &numModes, pModes))) {
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

    //devices_.push_back((GraphicsDeviceBase*)new GraphicsDeviceDirectX11Win());
  }

  for (IDXGIAdapter1* adapter : adapters) {
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
