#include "pch.h"
#include "audio/AudioBase.h"

namespace Mana {

void AudioBase::StopAll() {
  for (const auto& pair : m_fileMap) {
    Stop(pair.first);
  }
}

void AudioBase::PauseAll() {
  for (const auto& pair : m_fileMap) {
    Pause(pair.first);
  }
}

void AudioBase::ResumeAll() {
  for (const auto& pair : m_fileMap) {
    Resume(pair.first);
  }
}

AudioFileHandle AudioBase::GetNextFreeAudioFileHandle() {
  AudioFileHandle next = m_lastAudioFileHandle + 1;
  if (next > MAX_SOUNDS_LOADED)
    next = 1;  // wrap around

  // make sure next value isn't in use
  bool firstPass = true;
  while (true) {
    if (m_fileMap.find(next) == m_fileMap.end())
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

  m_lastAudioFileHandle = next;
  return next;
}

void AudioBase::GetStreamingFiles() {
  streamingFiles_.clear();
  for (auto const& item : m_fileMap) {
    if (item.second->loadType == AudioLoadType::Streaming) {
      streamingFiles_.push_back(item.second);
    }
  }
}

AudioFileBase* AudioBase::GetAudioFile(AudioFileHandle audioFileHandle) {
  auto search = m_fileMap.find(audioFileHandle);
  if (search == m_fileMap.end()) {
    return nullptr;
  }

  return search->second;
}

}  // namespace Mana
