#pragma once

#include "ManaGlobals.h"
#include "input/InputBase.h"

namespace Mana {

// Win32 Raw Input API wrapper.
// Allows to use multiple Keyboard and Mouse devices for separate players.
// See: https://devblogs.microsoft.com/oldnewthing/20160627-00/?p=93755
// Maybe useful: https://github.com/ytyaru/HelloRawInput20160702/blob/master/HelloRawInput20160702/Program.cpp
class RawInputWin {
 public:
  RawInputWin(HWND hwndTarget);
  virtual ~RawInputWin() = default;

  RawInputWin(const RawInputWin&) = delete;
  RawInputWin& operator=(const RawInputWin&) = delete;

  bool Init();
  bool Uninit();

  bool OnRawInput(HRAWINPUT hRawInput);
  bool OnInputDeviceChange(InputDeviceChangeType deviceChangeType,
                           U64 deviceId);

 private:
  HWND hwndTarget_;
  
  void* pRawInput_;
  size_t rawInputSizeBytes_;

  bool RegisterDevices();
  bool UnregisterDevices();
  
  bool ReallocRawInputPtr(size_t size);
};

}  // namespace Mana
