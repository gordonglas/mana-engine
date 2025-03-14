// base Audio Engine class

#pragma once

#include <fstream>
#include <map>
#include <string>
#include <vector>
#include "ManaGlobals.h"
#include "audio/AudioFileBase.h"
#include "concurrency/IThread.h"
#include "utils/File.h"

namespace Mana {

class AudioBase;
extern AudioBase* g_pAudioEngine;

class AudioBase {
 public:
  static const unsigned MAX_LOOP_COUNT = 254;
  static const unsigned LOOP_INFINITE = 255;

  AudioBase() = default;
  // TODO: maybe in debug build, assert if there are still files loaded?
  virtual ~AudioBase() = default;

  AudioBase(const AudioBase&) = delete;
  AudioBase& operator=(const AudioBase&) = delete;

  virtual bool Init() = 0;
  virtual void Uninit() = 0;

  // Returns non-zero for success.
  virtual AudioFileHandle Load(const xstring& filePath,
                               AudioCategory category,
                               AudioFormat format,
                               int64_t loopBackPcmSamplePos = 0,
                               int simultaneousSounds = 1) = 0;
  // stops and destroys all voices and the buffer
  // and removes from fileMap_
  virtual void Unload(AudioFileHandle audioFileHandle) = 0;

  // fills/queues streaming buffers, if needed
  virtual void Update() = 0;

  virtual bool Play(AudioFileHandle audioFileHandle,
                    uint32_t loopCount = 0) = 0;

  virtual void Stop(AudioFileHandle audioFileHandle) = 0;
  void StopAll();

  // Pause only works for files that only have 1 simultaneous sound
  // (doesn't make sense otherwise)
  // Call Play or Resume to continue playing.
  virtual void Pause(AudioFileHandle audioFileHandle) = 0;
  // Call ResumeAll to continue playing all paused voices.
  void PauseAll();

  virtual void Resume(AudioFileHandle audioFileHandle) = 0;
  void ResumeAll();

  virtual float GetVolume(AudioFileHandle audioFileHandle) = 0;
  // get average volume of sounds in the category,
  // since some may have been set individually
  virtual float GetVolume(AudioCategory category) = 0;
  virtual float GetMasterVolume() = 0;

  // allows developers to tweak volumes of individual files
  virtual void SetVolume(AudioFileHandle audioFileHandle, float volume) = 0;
  // allows users to set per-category volumes
  virtual void SetVolume(AudioCategory category, float volume) = 0;
  // allows users to set master volume
  virtual void SetMasterVolume(float& volume) = 0;

  // uses simple linear panning
  // pan of -1.0 is all left, 0 is middle, 1.0 is all right.
  virtual void SetPan(AudioFileHandle audioFileHandle, float pan) = 0;
  virtual float GetPan(AudioFileHandle audioFileHandle) = 0;

  // returns true if at least one voice of this sound is playing
  virtual bool IsPlaying(AudioFileHandle audioFileHandle) = 0;
  virtual bool IsPaused(AudioFileHandle audioFileHandle) = 0;

 protected:
  // this does not include simultaneous
  // versions of the same sound
  const AudioFileHandle MAX_SOUNDS_LOADED = 500;

  std::map<AudioFileHandle, AudioFileBase*> fileMap_;

  AudioFileHandle lastAudioFileHandle_ = 0;
  AudioFileHandle GetNextFreeAudioFileHandle();

  std::vector<AudioFileBase*> streamingFiles_;
  void GetStreamingFiles();

  AudioFileBase* GetAudioFile(AudioFileHandle audioFileHandle);
  virtual void ClampVolume(float& volume) = 0;
};

}  // namespace Mana
