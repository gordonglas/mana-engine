// base Graphics Engine class

#pragma once

#include <vector>
#include "ManaGlobals.h"
#include "graphics/GraphicsDeviceBase.h"

namespace Mana {

class GraphicsBase;
extern GraphicsBase* g_pGraphicsEngine;

struct GraphicsMode {
  bool isPrimaryDisplayMode;
  
};

struct GraphicsRect {
  long left;
  long top;
  long right;
  long bottom;
};

enum class GraphicsOutputRotation {
  Unspecified = 0,
  Identity = 1,
  Rotate90 = 2,
  Rotate180 = 3,
  Rotate270 = 4
};

struct GraphicsAdaptorOutput {
  xstring name;
  GraphicsRect desktopCoordinates;
  bool isAttachedToDesktop;
  GraphicsOutputRotation rotation;
  std::vector<GraphicsMode> modes;
};

struct GraphicsAdaptor {
  xstring name;
  size_t dedicatedVideoMemory;
  size_t dedicatedSystemMemory;
  size_t sharedSystemMemory;
  bool hasSoftwareRenderer;
  std::vector<GraphicsAdaptorOutput> outputs;
};

class GraphicsBase {
 public:
  GraphicsBase() {}
  virtual ~GraphicsBase() {}

  virtual bool Init() = 0;
  virtual void Uninit() = 0;

  // Refreshes list of video adapters that have outputs
  // and gets their supported fullscreen modes.
  virtual bool EnumerateAdaptersAndFullScreenModes() = 0;

 protected:
  std::vector<GraphicsDeviceBase*> devices_;
};

}  // namespace Mana
