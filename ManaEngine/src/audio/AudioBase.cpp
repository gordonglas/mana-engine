#include "pch.h"
#include "audio/AudioBase.h"

namespace Mana {

void AudioBase::StopAll() {
  for (const auto& pair : fileMap_) {
    Stop(pair.first);
  }
}

void AudioBase::PauseAll() {
  for (const auto& pair : fileMap_) {
    Pause(pair.first);
  }
}

void AudioBase::ResumeAll() {
  for (const auto& pair : fileMap_) {
    Resume(pair.first);
  }
}

AudioFileHandle AudioBase::GetNextFreeAudioFileHandle() {
  AudioFileHandle next = lastAudioFileHandle_ + 1;
  if (next > MAX_SOUNDS_LOADED)
    next = 1;  // wrap around

  // make sure next value isn't in use
  bool firstPass = true;
  while (true) {
    if (fileMap_.find(next) == fileMap_.end())
      break;

    next++;
    if (next > MAX_SOUNDS_LOADED) {
      if (!firstPass) {
        return 0;  // too many sounds loaded.
      }

      firstPass = false;
      next = 1;
    }
  }

  lastAudioFileHandle_ = next;
  return next;
}

void AudioBase::GetStreamingFiles() {
  streamingFiles_.clear();
  for (auto const& item : fileMap_) {
    if (item.second->loadType_ == AudioLoadType::Streaming) {
      streamingFiles_.push_back(item.second);
    }
  }
}

AudioFileBase* AudioBase::GetAudioFile(AudioFileHandle audioFileHandle) {
  auto search = fileMap_.find(audioFileHandle);
  if (search == fileMap_.end()) {
    return nullptr;
  }

  return search->second;
}

}  // namespace Mana
