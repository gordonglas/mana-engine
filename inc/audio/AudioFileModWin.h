// Windows-specific Mod file format details

#pragma once

#include <libopenmpt/libopenmpt.hpp>
#include "ManaGlobals.h"
#include "audio/AudioFileWin.h"
#include "target/TargetOS.h"

namespace Mana {

class AudioFileModWin : public AudioFileWin {
 public:
  AudioFileModWin();
  // dtor doesn't stop sounds or destroy xaudio2 buffers.
  // The Audio engine handles this.
  ~AudioFileModWin() override;

  openmpt::module* modLib;

  bool Load(const xstring& strFilePath) override;
  void Unload() override;
};

}  // namespace Mana
