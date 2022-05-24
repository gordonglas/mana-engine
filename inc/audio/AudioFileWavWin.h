// Windows-specific Wav file format details

#pragma once

#include "ManaGlobals.h"
#include "audio/AudioFileWin.h"
#include "target/TargetOS.h"

namespace Mana {

class AudioFileWavWin : public AudioFileWin {
 public:
  AudioFileWavWin();
  // dtor doesn't stop sounds or destroy xaudio2 buffers.
  // The Audio engine handles this.
  ~AudioFileWavWin();

  bool Load(const xstring& strFilePath) override;
};

}  // namespace Mana