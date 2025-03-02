// base Audio File class

#pragma once

#include "ManaGlobals.h"

namespace Mana {

// Assumes streaming files' uncompressed pcm data
// is larger than (AudioStreamBufSize * AudioStreamBufCount),
// else just load it as an uncompressed pcm wav file.
// Should always be a multiple of 4 (in case we want to use 32 bit samples).
constexpr size_t AudioStreamBufSize = 65536;
constexpr size_t AudioStreamBufCount = 3;

typedef size_t AudioFileHandle;

enum class AudioCategory { Sound, Music, Voice };
enum class AudioLoadType { Static, Streaming };
enum class AudioFormat { Wav, Ogg };

// internal class used by the Audio engine
class AudioFileBase {
 public:
  AudioFileBase();
  // dtor doesn't stop sounds or destroy audio buffers.
  // The Audio engine handles this.
  virtual ~AudioFileBase();

  AudioFileBase(const AudioFileBase&) = delete;
  AudioFileBase& operator=(const AudioFileBase&) = delete;

  AudioFileHandle audioFileHandle_;
  xstring filePath_;
  AudioCategory category_;
  AudioLoadType loadType_;
  AudioFormat format_;
  float pan_;
  bool isPaused_;
  bool isStopped_;
  size_t fileSize_;               // raw file size
  uint8_t* pDataBuffer_;          // pcm buffer
  size_t dataBufferSize_;         // pcm buffer size
  size_t currentStreamBufIndex_;  // 0 to (AudioStreamBufCount - 1)
  size_t totalPcmBytes_;          // total size of pcm data in entire file
  size_t currentTotalPcmPos_;     // current position within totalPcmBytes

  bool lastBufferPlaying_;

  uint32_t loopCount_;  // Number of times to repeat the entire sound,
                        // or AudioBase::LOOP_INFINITE to loop until stopped.
                        // If > 0 and != AudioBase::LOOP_INFINITE,
                        // gets decremented over time.
  int64_t loopBackPcmSamplePos_; // pcm pos to loop back to, in samples.
                                 // Must be on a pcm frame boundary.

  virtual bool Load(const xstring& strFilePath) = 0;
  virtual void Unload() = 0;

  // streaming-only functions
  virtual bool StreamSeek(int64_t pcmBytePos) = 0;
};

}  // namespace Mana
