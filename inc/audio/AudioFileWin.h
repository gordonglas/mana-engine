// Windows-specific Audio File members

#pragma once

#include <xaudio2.h>
#include "ManaGlobals.h"
#include "audio/AudioFileBase.h"
#include "target/TargetOS.h"

namespace Mana {

class AudioFileWin : public AudioFileBase {
 public:
  AudioFileWin();
  // dtor doesn't stop sounds or destroy xaudio2 buffers.
  // The Audio engine handles this.
  ~AudioFileWin() override;

  WAVEFORMATEXTENSIBLE wfx;
  std::vector<IXAudio2SourceVoice*> sourceVoices;
  size_t sourceVoicePos;

  // XAUDIO2_BUFFER buffer;
};

}  // namespace Mana
