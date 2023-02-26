#pragma once

#include "ManaGlobals.h"
#include "utils/File.h"

namespace Mana {

// Manages game configuration files.
class ConfigManager {
 public:
  ConfigManager();
  ~ConfigManager();

  bool LoadDefaultConfig(File* pFile);

};

}
