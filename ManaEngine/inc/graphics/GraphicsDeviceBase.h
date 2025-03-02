// base Graphics Device class

#pragma once

namespace Mana {

struct MultisampleLevel {
  U32 SampleCount;
  U32 QualityLevels;
};

class GraphicsDeviceBase {
 public:
  GraphicsDeviceBase() = default;
  virtual ~GraphicsDeviceBase() = default;

  GraphicsDeviceBase(const GraphicsDeviceBase&) = delete;
  GraphicsDeviceBase& operator=(const GraphicsDeviceBase&) = delete;

  virtual bool Init() = 0;
  virtual void Uninit() = 0;

  virtual bool GetSupportedMultisampleLevels(
      std::vector<MultisampleLevel>& levels) = 0;

  xstring name_;
};

}  // namespace Mana
