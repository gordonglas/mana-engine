#pragma once

#include "ManaGlobals.h"
#include "utils/File.h"

namespace Mana {

// Manages game configuration files.
class ConfigManager {
 public:
  ConfigManager() = default;
  virtual ~ConfigManager() = default;

  ConfigManager(const ConfigManager&) = delete;
  ConfigManager& operator=(const ConfigManager&) = delete;

  bool LoadDefaultConfig(File* pFile);

};

}  // namespace Mana
