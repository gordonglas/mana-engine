// Shared DirectX11 headers

#pragma once

#include "target/TargetOS.h"

// dxgi1_4 was introduced in Windows 8.1
// dxgi1_5 was introduced in Windows 10, version 1903 (10.0; Build 18362)
// Since we're supporting Win10+, we'll use dxgi1_4.
// dxgi1_4 adds the newer flip swap chain modes for better windowed-mode performance.
// See: https://walbourn.github.io/care-and-feeding-of-modern-swapchains/
//      https://stackoverflow.com/questions/42354369/idxgifactory-versions
#include <dxgi1_4.h>

// Require DirectX 11.3 (Requires Windows 10+)
// See: https://walbourn.github.io/anatomy-of-direct3d-11-create-device/
#include <d3d11_3.h>

// ComPtr<T> - See: https://github.com/Microsoft/DirectXTK/wiki/ComPtr
#include <wrl/client.h>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxguid.lib")

#include "graphics/DirectX11DebugLayer.h"
