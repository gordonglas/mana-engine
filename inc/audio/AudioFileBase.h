// base Audio File class

#pragma once

#include "ManaGlobals.h"
#include "target/TargetOS.h"

namespace Mana {

typedef size_t AudioFileHandle;

enum class AudioCategory { Sound, Music, Voice };
enum class AudioLoadType { Static, Streaming };
enum class AudioFormat { Wav, Ogg, Mod };

// internal class used by the Audio engine
class AudioFileBase {
 protected:
  AudioFileBase();
  // dtor doesn't stop sounds or destroy audio buffers.
  // The Audio engine handles this.
  virtual ~AudioFileBase();

 public:
  AudioFileHandle audioFileHandle;
  xstring filePath;
  AudioCategory category;
  AudioLoadType loadType;
  AudioFormat format;
  float pan;
  bool isPaused;
  size_t fileSize;  // raw file size
  // WAVEFORMATEXTENSIBLE wfx;
  BYTE* pDataBuffer;             // pcm buffer
  size_t dataBufferSize;         // pcm buffer size
  size_t currentStreamBufIndex;  // 0 to (AudioStreamBufCount - 1)
  size_t currentBufPos;          // 0 to (dataBufferSize - 1)

  bool lastBufferPlaying;

  UINT32 playBegin;   // First sample in this buffer to be played.
  UINT32 playLength;  // Length of the region to be played in samples,
                      //  or 0 to play the whole buffer.
  UINT32 loopBegin;   // First sample of the region to be looped.
  UINT32 loopLength;  // Length of the desired loop region in samples,
                      //  or 0 to loop the entire buffer.
  UINT32 loopCount;   // Number of times to repeat the loop region,
                      //  or XAUDIO2_LOOP_INFINITE to loop forever.

  virtual bool Load(const xstring& strFilePath) = 0;
  virtual void Unload() {};
};

}  // namespace Mana
