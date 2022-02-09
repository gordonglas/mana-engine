#include "pch.h"
#include "audio/AudioBase.h"

namespace Mana {

AudioFileBase::AudioFileBase() {
  audioFileHandle = 0;
  category = AudioCategory::Sound;
  loadType = AudioLoadType::Static;
  format = AudioFormat::Wav;
  pan = 0.0f;
  isPaused = false;
  // wfx = { 0 };
  pDataBuffer = nullptr;
  dataBufferSize = 0;
  playBegin = 0;
  playLength = 0;
  loopBegin = 0;
  loopLength = 0;
  loopCount = 0;
  // sourceVoicePos = 0;
  currentStreamBufIndex = 0;
  currentBufPos = 0;
  fileSize = 0;
  lastBufferPlaying = false;
  lib = nullptr;
}

AudioFileBase::~AudioFileBase() {
  if (pDataBuffer) {
    delete[] pDataBuffer;
    pDataBuffer = nullptr;
  }
}

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

std::vector<AudioFileBase*> AudioBase::GetStreamingFiles() {
  std::vector<AudioFileBase*> streamingFiles;

  for (auto const& item : m_fileMap) {
    if (item.second->loadType == AudioLoadType::Streaming) {
      streamingFiles.push_back(item.second);
    }
  }

  return streamingFiles;
}

AudioFileBase* AudioBase::GetAudioFile(AudioFileHandle audioFileHandle) {
  auto search = m_fileMap.find(audioFileHandle);
  if (search == m_fileMap.end()) {
    return nullptr;
  }

  return search->second;
}

}  // namespace Mana
