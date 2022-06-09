// base Audio File class

#pragma once

#include "ManaGlobals.h"
#include "target/TargetOS.h"

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

  AudioFileHandle audioFileHandle;
  xstring filePath;
  AudioCategory category;
  AudioLoadType loadType;
  AudioFormat format;
  float pan;
  bool isPaused;
  bool isStopped;
  size_t fileSize;               // raw file size
  BYTE* pDataBuffer;             // pcm buffer
  size_t dataBufferSize;         // pcm buffer size
  size_t currentStreamBufIndex;  // 0 to (AudioStreamBufCount - 1)
  size_t totalPcmBytes;          // total size of pcm data in entire file
  size_t currentTotalPcmPos;     // current position within totalPcmBytes

  bool lastBufferPlaying;

  UINT32 loopCount;   // Number of times to repeat the entire sound,
                      //  or AudioBase::LOOP_INFINITE to loop until stopped.
                      // If > 0 and != AudioBase::LOOP_INFINITE,
                      // gets decremented over time.

  virtual bool Load(const xstring& strFilePath) = 0;
  virtual void Unload() = 0;

  // streaming-only functions
  virtual void ResetToStartPos() = 0;
};

}  // namespace Mana
