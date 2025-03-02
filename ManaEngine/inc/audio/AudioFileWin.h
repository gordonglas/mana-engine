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
  virtual ~AudioFileWin() = default;

  AudioFileWin(const AudioFileWin&) = delete;
  AudioFileWin& operator=(const AudioFileWin&) = delete;

  WAVEFORMATEXTENSIBLE wfx_;
  std::vector<IXAudio2SourceVoice*> sourceVoices_;
  size_t sourceVoicePos_;

  //XAUDIO2_BUFFER buffer_;

  virtual bool Load(const xstring& strFilePath) override = 0;
  virtual void Unload() override = 0;

  virtual bool StreamSeek(int64_t pcmBytePos) override = 0;
};

}  // namespace Mana
