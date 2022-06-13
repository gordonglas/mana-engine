// Windows-specific Audio Engine functionality

#pragma once

#include <xaudio2.h>
#include "ManaGlobals.h"
#include "audio/AudioBase.h"
#include "audio/AudioFileWin.h"
#include "target/TargetOS.h"

//#pragma comment(lib, "xaudio2_9redist.lib")

namespace Mana {

// XAudio2.9 wrapper with focus on simple indie games.
// Can load: static uncompressed wav, streaming ogg vorbis, and various mod
// formats.
class AudioWin : public AudioBase {
 public:
  static const unsigned MAX_LOOP_COUNT = XAUDIO2_MAX_LOOP_COUNT;
  static const unsigned LOOP_INFINITE = XAUDIO2_LOOP_INFINITE;

  bool Init() override;
  void Uninit() override;
  AudioFileHandle Load(xstring filePath,
                       AudioCategory category,
                       AudioFormat format,
                       int simultaneousSounds = 1) override;
  void Unload(AudioFileHandle audioFileHandle) override;

  void Update() override;

  bool Play(AudioFileHandle audioFileHandle,
            unsigned loopCount = 0) override;

  void Stop(AudioFileHandle audioFileHandle) override;
  void Pause(AudioFileHandle audioFileHandle) override;
  void Resume(AudioFileHandle audioFileHandle) override;

  float GetVolume(AudioFileHandle audioFileHandle) override;
  float GetVolume(AudioCategory category) override;
  float GetMasterVolume() override;

  void SetVolume(AudioFileHandle audioFileHandle, float volume) override;
  void SetVolume(AudioCategory category, float volume) override;
  void SetMasterVolume(float& volume) override;

  void SetPan(AudioFileHandle audioFileHandle, float pan) override;
  float GetPan(AudioFileHandle audioFileHandle) override;

  bool IsPlaying(AudioFileHandle audioFileHandle) override;
  bool IsPaused(AudioFileHandle audioFileHandle) override;

 private:
  // 0 is silent
  const float AUDIO_MIN_VOLUME = 0.0f;
  // 1.0 is no attenuation or gain.
  // Setting it any higher can result in clipping,
  // although clipping can occur at 1.0f as well.
  // See: https://stackoverflow.com/a/40223133
  const float AUDIO_MAX_VOLUME = 1.0f;

  IXAudio2* m_pXAudio2 = nullptr;
  IXAudio2MasteringVoice* m_pMasterVoice = nullptr;

  void ClampVolume(float& volume) override;
};

}  // namespace Mana

// speaker layout macros that correspond to WAVEFORMATEXTENSIBLE format
// used for panning

#ifndef SPEAKER_FRONT_LEFT
#define SPEAKER_FRONT_LEFT            0x00000001
#define SPEAKER_FRONT_RIGHT           0x00000002
#define SPEAKER_FRONT_CENTER          0x00000004
#define SPEAKER_LOW_FREQUENCY         0x00000008
#define SPEAKER_BACK_LEFT             0x00000010
#define SPEAKER_BACK_RIGHT            0x00000020
#define SPEAKER_FRONT_LEFT_OF_CENTER  0x00000040
#define SPEAKER_FRONT_RIGHT_OF_CENTER 0x00000080
#define SPEAKER_BACK_CENTER           0x00000100
#define SPEAKER_SIDE_LEFT             0x00000200
#define SPEAKER_SIDE_RIGHT            0x00000400
#define SPEAKER_TOP_CENTER            0x00000800
#define SPEAKER_TOP_FRONT_LEFT        0x00001000
#define SPEAKER_TOP_FRONT_CENTER      0x00002000
#define SPEAKER_TOP_FRONT_RIGHT       0x00004000
#define SPEAKER_TOP_BACK_LEFT         0x00008000
#define SPEAKER_TOP_BACK_CENTER       0x00010000
#define SPEAKER_TOP_BACK_RIGHT        0x00020000
#define SPEAKER_RESERVED              0x7FFC0000
#define SPEAKER_ALL                   0x80000000
#define _SPEAKER_POSITIONS_
#endif

#ifndef SPEAKER_STEREO
#define SPEAKER_MONO                (SPEAKER_FRONT_CENTER)
#define SPEAKER_STEREO              (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT)
#define SPEAKER_2POINT1             (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY)
#define SPEAKER_SURROUND            (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_BACK_CENTER)
#define SPEAKER_QUAD                (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT)
#define SPEAKER_4POINT1             (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT)
#define SPEAKER_5POINT1             (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT)
#define SPEAKER_7POINT1             (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_FRONT_LEFT_OF_CENTER | SPEAKER_FRONT_RIGHT_OF_CENTER)
#define SPEAKER_5POINT1_SURROUND    (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT)
#define SPEAKER_7POINT1_SURROUND    (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT)
#endif
