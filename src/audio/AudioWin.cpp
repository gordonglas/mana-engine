#include "pch.h"
#include <assert.h>
#include "audio/AudioWin.h"
#include "audio/AudioFileBase.h"
#include "audio/AudioFileOggWin.h"
#include "concurrency/IThread.h"

namespace Mana {

AudioBase* g_pAudioEngine = nullptr;

// TODO: move this somewhere else. (into logger?)
std::wstring GetDateTimeNow() {
  SYSTEMTIME st;
  GetSystemTime(&st);
  char currentTime[84] = "";
  sprintf_s(currentTime, "%.4d%.2d%.2d%.2d%.2d%.2d%.4d", st.wYear, st.wMonth,
            st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
  return std::wstring(Utf8ToUtf16(currentTime));
}

// Requires call to CoInitialize. Call: utils/Com.h::Init.
bool AudioWin::Init() {
  m_pXAudio2 = nullptr;
  m_pMasterVoice = nullptr;

  HRESULT hr;
  if (FAILED(hr = XAudio2Create(&m_pXAudio2, 0, XAUDIO2_USE_DEFAULT_PROCESSOR)))
    return false;

  if (FAILED(hr = m_pXAudio2->CreateMasteringVoice(&m_pMasterVoice)))
    return false;

  m_lastAudioFileHandle = 0;

  return true;
}

void AudioWin::Uninit() {
  // stop and destroy all voices and buffers by calling Unload on all AudioFiles

  if (m_pXAudio2) {
    m_pXAudio2->StopEngine();
  }

  // since Unload calls "m_fileMap.erase",
  // and therefore alters the foreach iterator,
  // first form a list of audio handles,
  // then use those to call Unload.
  std::vector<AudioFileHandle> fileHandleList;
  for (auto it = m_fileMap.begin(); it != m_fileMap.end(); ++it) {
    AudioFileBase* pAudioFile = it->second;
    fileHandleList.push_back(pAudioFile->audioFileHandle);
  }

  for (size_t i = 0; i < fileHandleList.size(); ++i) {
    Unload(fileHandleList[i]);
  }

  if (m_pMasterVoice) {
    m_pMasterVoice->DestroyVoice();
    m_pMasterVoice = nullptr;
  }

  if (m_pXAudio2) {
    m_pXAudio2->Release();
    m_pXAudio2 = nullptr;
  }
}

AudioFileHandle AudioWin::Load(xstring filePath,
                               AudioCategory category,
                               AudioFormat format,
                               int simultaneousSounds) {
  AudioFileHandle audioFileHandle = 0;

  if (simultaneousSounds < 1)
    return 0;

  AudioFileWin* pFile = nullptr;
  if (format == AudioFormat::Ogg) {
    pFile = new AudioFileOggWin;
  }

  if (!pFile)
    return 0;

  pFile->filePath = filePath;
  pFile->category = category;
  pFile->format = format;

  if (!pFile->Load(filePath)) {
    delete pFile;
    return 0;
  }

  if (simultaneousSounds > 1 && pFile->loadType == AudioLoadType::Streaming) {
    assert(false &&
           "Audio file is too big to support multiple simultaneous sounds");
    delete pFile;
    return 0;
  }

  audioFileHandle = GetNextFreeAudioFileHandle();
  if (!audioFileHandle) {
    delete pFile;
    return 0;
  }

  pFile->audioFileHandle = audioFileHandle;

  m_fileMap.insert(std::map<AudioFileHandle, AudioFileBase*>::value_type(
      audioFileHandle, pFile));

  int numSounds = simultaneousSounds;
  while (numSounds > 0) {
    IXAudio2SourceVoice* pSourceVoice = nullptr;

    //if (pFile->loadType == AudioLoadType::Streaming) {
    //  if (FAILED(m_pXAudio2->CreateSourceVoice(
    //          &pSourceVoice, (WAVEFORMATEX*)&pFile->wfx, 0u, 2.0f,
    //          (IXAudio2VoiceCallback*)&voiceCallback))) {
    //    OutputDebugStringW(L"ERROR: CreateSourceVoice streaming failed\n");
    //    delete pFile;
    //    return 0;
    //  }
    //} else {
      if (FAILED(m_pXAudio2->CreateSourceVoice(&pSourceVoice,
                                               (WAVEFORMATEX*)&pFile->wfx))) {
        OutputDebugStringW(L"ERROR: CreateSourceVoice failed\n");
        delete pFile;
        return 0;
      }
    //}

    pFile->sourceVoices.push_back(pSourceVoice);
    numSounds--;
  }

  // set defaults
  pFile->sourceVoicePos = 0;
  pFile->isPaused = false;
  pFile->pan = 0.0f;

  return audioFileHandle;
}

void AudioWin::Unload(AudioFileHandle audioFileHandle) {
  AudioFileWin* pAudioFile = (AudioFileWin*)GetAudioFile(audioFileHandle);
  if (!pAudioFile) {
    return;
  }

  for (size_t i = 0; i < pAudioFile->sourceVoices.size(); ++i) {
    auto* pSourceVoice = pAudioFile->sourceVoices[i];

    // DestroyVoice waits for the XAudio2 audio processing thread to be
    // idle, so it can take a little while (typically no more than a
    // couple of milliseconds). This is necessary to guarantee that the
    // voice will no longer make any callbacks or read any audio data,
    // so the application can safely free up these resources as soon
    // as the call returns.
    pSourceVoice->DestroyVoice();
    pSourceVoice = nullptr;
  }

  m_fileMap.erase(pAudioFile->audioFileHandle);

  pAudioFile->sourceVoices.clear();

  pAudioFile->Unload();

  delete pAudioFile;
  pAudioFile = nullptr;
}

void AudioWin::Update() {
  XAUDIO2_VOICE_STATE voiceState;
  XAUDIO2_BUFFER buffer;
  std::vector<AudioFileBase*> streamingFiles = GetStreamingFiles();

  for (AudioFileBase* pFileBase : streamingFiles) {
    AudioFileWin* pFile = static_cast<AudioFileWin*>(pFileBase);

    if (pFile->isStopped)
      continue;

    if (pFile->lastBufferPlaying) {
      // last buffer just finished playing.
      OutputDebugStringW(L"file done playing\n");
      pFile->currentStreamBufIndex = 0;
      pFile->ResetToStartPos();
      pFile->isStopped = true;
      continue;
    }

    // NOTE: even if this streaming sound is paused,
    // we still want to fill it's buffers and queue them on it's
    // XAudio2-voice. They just won't play until the sound is resumed.

    int bytesPerSample = pFile->wfx.Format.wBitsPerSample / 8;

    // get number of buffers currently in the XAudio2-Voice queue
    pFile->sourceVoices[0]->GetState(&voiceState,
                                     XAUDIO2_VOICE_NOSAMPLESPLAYED);

    while (voiceState.BuffersQueued < AudioStreamBufCount) {
      OutputDebugStringW((std::wstring(L"BuffersQueued: ") +
                          std::to_wstring(voiceState.BuffersQueued) + L"\n")
                             .c_str());
      //assert(voiceState.BuffersQueued > 0 && "BuffersQueued == 0!");

      // init next buffer with silence
      memset(
          &pFile
               ->pDataBuffer[pFile->currentStreamBufIndex * AudioStreamBufSize],
          0, AudioStreamBufSize);

      buffer = {0};

      int readBufLen = AudioStreamBufSize;  // multiple of 4

      size_t destBufPos = pFile->currentStreamBufIndex * AudioStreamBufSize;
      unsigned currentBytesRead = 0;
      long actualBytesRead = 1;
      int ovBitstream = 0;

      int bytesToPcmEOF =
          (int)(pFile->totalPcmBytes - pFile->currentTotalPcmPos);
      // OutputDebugStringW((std::wstring(L"Audio Update: bytesToPcmEOF: ") +
      //                     std::to_wstring(bytesToPcmEOF) + L"\n")
      //                        .c_str());
      bool reachedEOF = false;
      if (bytesToPcmEOF < readBufLen) {
        readBufLen = bytesToPcmEOF;
        reachedEOF = true;
        OutputDebugStringW(L"Audio Update: reachedEOF true\n");
      }

      size_t maxBytesToRead = reachedEOF ? bytesToPcmEOF : AudioStreamBufSize;

      AudioFileOggWin* pOggFile = static_cast<AudioFileOggWin*>(pFile);

      while (actualBytesRead && currentBytesRead < maxBytesToRead) {
        actualBytesRead = ::ov_read(
            &pOggFile->oggVorbisFile_, (char*)&pFile->pDataBuffer[destBufPos],
            readBufLen, 0, bytesPerSample, 1, &ovBitstream);
        OutputDebugStringW(
            (std::wstring(L"Audio Update: ov_read actualBytesRead: ") +
             std::to_wstring(actualBytesRead) + L"\n")
                .c_str());
        assert(actualBytesRead >= 0 && "ov_read failed");
        destBufPos += actualBytesRead;
        currentBytesRead += actualBytesRead;
        pFile->currentTotalPcmPos += actualBytesRead;
        OutputDebugStringW((std::wstring(L"Audio Update: ov_read pcmPos: ") +
                            std::to_wstring(pFile->currentTotalPcmPos) + L"\n")
                               .c_str());
        if (maxBytesToRead - currentBytesRead < (unsigned)readBufLen) {
          readBufLen = (int)maxBytesToRead - currentBytesRead;
        }
      }

      // bool onLastLoop = false;
      if (reachedEOF && pFile->loopCount > 0 &&
          pFile->loopCount != AudioBase::LOOP_INFINITE) {
        pFile->loopCount--;
        // if (pFile->loopCount == 0) {
        //   onLastLoop = true;
        // }
      }

      // if reached the end of file and still looping,
      // reset position to start of the file,
      // and fill the rest of the destination buffer.
      if (reachedEOF && pFile->loopCount > 0) {
        pFile->currentTotalPcmPos = 0;

        // seek to start of compressed ogg file
        int seekRet = ::ov_raw_seek(&pOggFile->oggVorbisFile_, 0);
        OutputDebugStringW(
            (std::wstring(L"Audio Update: ov_raw_seek returned: ") +
             std::to_wstring(seekRet) + L"\n")
                .c_str());

        if (currentBytesRead < AudioStreamBufSize) {
          // fill the rest of the destination buffer
          readBufLen = AudioStreamBufSize - currentBytesRead;
          actualBytesRead = 1;
          while (actualBytesRead && currentBytesRead < AudioStreamBufSize) {
            actualBytesRead =
                ::ov_read(&pOggFile->oggVorbisFile_,
                          (char*)&pFile->pDataBuffer[destBufPos], readBufLen, 0,
                          bytesPerSample, 1, &ovBitstream);
            OutputDebugStringW(
                (std::wstring(L"Audio Update: ov_read actualBytesRead: ") +
                 std::to_wstring(actualBytesRead) + L"\n")
                    .c_str());
            assert(actualBytesRead >= 0 && "ov_read failed");
            destBufPos += actualBytesRead;
            currentBytesRead += actualBytesRead;
            pFile->currentTotalPcmPos += actualBytesRead;
            if (AudioStreamBufSize - currentBytesRead < readBufLen) {
              readBufLen = AudioStreamBufSize - currentBytesRead;
            }
          }
        }
      } else {  // not looping (or on last loop)
        if (reachedEOF) {
          pFile->lastBufferPlaying = true;
          OutputDebugStringW(L"Audio Update: set lastBufferPlaying true\n");
        }
      }

      buffer.AudioBytes = currentBytesRead;
      buffer.pAudioData =
          &pFile
               ->pDataBuffer[pFile->currentStreamBufIndex * AudioStreamBufSize];
      if (pFile->lastBufferPlaying) {
        buffer.Flags = XAUDIO2_END_OF_STREAM;
      }

      pFile->currentStreamBufIndex++;
      if (pFile->currentStreamBufIndex == AudioStreamBufCount) {
        pFile->currentStreamBufIndex = 0;
      }

      HRESULT hr;
      if (FAILED(hr = pFile->sourceVoices[0]->SubmitSourceBuffer(&buffer))) {
        OutputDebugStringW(L"Audio Update: SubmitSourceBuffer failed\n");
        assert(false && "Audio Update: SubmitSourceBuffer failed");
        return;
      }

      if (pFile->lastBufferPlaying) {
        break;
      }

      pFile->sourceVoices[0]->GetState(&voiceState,
                                       XAUDIO2_VOICE_NOSAMPLESPLAYED);
    }
  }
}

bool AudioWin::Play(AudioFileHandle audioFileHandle, unsigned loopCount) {
  AudioFileWin* pFile = (AudioFileWin*)GetAudioFile(audioFileHandle);
  if (!pFile) {
    return false;
  }

  // if paused, resume
  if (pFile->isPaused) {
    Resume(audioFileHandle);
    return true;
  }

  // if streaming sound is already playing, do nothing.
  if (pFile->loadType == AudioLoadType::Streaming && !pFile->isPaused &&
      IsPlaying(audioFileHandle)) {
    return true;
  }

  IXAudio2SourceVoice* pSourceVoice =
      pFile->sourceVoices[pFile->sourceVoicePos];

  if (pFile->sourceVoicePos == pFile->sourceVoices.size() - 1) {
    pFile->sourceVoicePos = 0;
  } else {
    pFile->sourceVoicePos++;
  }

  assert(loopCount <= AudioBase::LOOP_INFINITE && "invalid loopCount");
  pFile->loopCount = loopCount;

  if (pFile->loadType == AudioLoadType::Static) {
    // It's ok for XAUDIO2_BUFFER objects to only be used temporarily.
    // Per docs here:
    // https://docs.microsoft.com/en-us/windows/win32/api/xaudio2/ns-xaudio2-xaudio2_buffer
    // "Memory allocated to hold a XAUDIO2_BUFFER or XAUDIO2_BUFFER_WMA
    // structure
    //  can be freed as soon as the IXAudio2SourceVoice::SubmitSourceBuffer call
    //  it is passed to returns." (except for the underlying data buffer,
    //  which we need for streaming anyway, which is
    //  "AudioFileBase::pDataBuffer")
    XAUDIO2_BUFFER buffer = {0};
    buffer.AudioBytes =
        (UINT32)pFile->dataBufferSize;  // size of the audio buffer in bytes
    buffer.pAudioData = pFile->pDataBuffer;  // buffer containing audio data
    buffer.Flags = XAUDIO2_END_OF_STREAM;    // tell the source voice not to
                                           // expect any data after this buffer

    if (FAILED(pSourceVoice->SubmitSourceBuffer(&buffer))) {
      OutputDebugStringW(L"ERROR: Play SubmitSourceBuffer failed!\n");
      return false;
    }
  } else  // streaming
  {
    AudioFileOggWin* pOggFile = static_cast<AudioFileOggWin*>(pFile);

    // Setup streaming buffers.
    // Since we're using the XAudio2 OnBufferEnd callback, we need to submit
    // more than 1 buffer to prevent a short silence when the first buffer
    // finishes playing. We will submit all |AudioStreamBufCount| buffers.

    pFile->currentStreamBufIndex = 0;
    pFile->ResetToStartPos();
    pFile->lastBufferPlaying = false;

    int bytesPerSample = pFile->wfx.Format.wBitsPerSample / 8;

    size_t bufIndex = 0;
    while (bufIndex < AudioStreamBufCount) {
      memset(&pFile->pDataBuffer[bufIndex * AudioStreamBufSize], 0,
             AudioStreamBufSize);

      // per "ov_read" docs,
      // the passed in buffer size is treated as a limit and not a request,
      // and if the passed in buffer is large, ov_read() will not fill it.
      // Therefore, we keep calling ov_read until we fill our (larger) buffer.

      int readBufLen = AudioStreamBufSize;  // multiple of 4

      size_t destBufPos = bufIndex * AudioStreamBufSize;
      unsigned int currentBytesRead = 0;
      long actualBytesRead = 1;
      int ovBitstream = 0;

      while (actualBytesRead && currentBytesRead < AudioStreamBufSize) {
        // NOTE: we're only supporting 2 channels, but if that changes,
        //   interleaved channel order is listed in the docs for other numbers
        //   of channels. https://xiph.org/vorbis/doc/vorbisfile/ov_read.html
        actualBytesRead = ::ov_read(
            &pOggFile->oggVorbisFile_, (char*)&pFile->pDataBuffer[destBufPos],
            readBufLen, 0, bytesPerSample, 1, &ovBitstream);
        OutputDebugStringW(
            (std::wstring(L"ogg file Play: ov_read actualBytesRead: ") +
             std::to_wstring(actualBytesRead) + L"\n")
                .c_str());
        assert(actualBytesRead >= 0 && "ov_read failed");
        destBufPos += actualBytesRead;
        currentBytesRead += actualBytesRead;
        pFile->currentTotalPcmPos += actualBytesRead;
        OutputDebugStringW(
            (std::wstring(L"ogg file Play: ov_read pcmPos: ") +
             std::to_wstring(pFile->currentTotalPcmPos) + L"\n")
                .c_str());
        if (AudioStreamBufSize - currentBytesRead < readBufLen) {
          readBufLen = AudioStreamBufSize - currentBytesRead;
        }
      }

      XAUDIO2_BUFFER buffer = {0};
      buffer.AudioBytes = (UINT32)currentBytesRead;
      buffer.pAudioData = &pFile->pDataBuffer[bufIndex * AudioStreamBufSize];
      buffer.Flags = 0;

      pFile->currentStreamBufIndex++;
      if (pFile->currentStreamBufIndex == AudioStreamBufCount) {
        pFile->currentStreamBufIndex = 0;
      }

      if (FAILED(pSourceVoice->SubmitSourceBuffer(&buffer))) {
        OutputDebugStringW(L"ERROR: SubmitSourceBuffer streaming failed\n");
        return 0;
      }

      ++bufIndex;
    }
  }

  if (FAILED(pSourceVoice->Start())) {
    OutputDebugStringW(L"ERROR: Play Start failed!\n");
    return false;
  }

  pFile->isPaused = false;
  pFile->isStopped = false;

  return true;
}

void AudioWin::Stop(AudioFileHandle audioFileHandle) {
  AudioFileWin* pAudioFile = (AudioFileWin*)GetAudioFile(audioFileHandle);
  if (!pAudioFile || pAudioFile->isStopped) {
    return;
  }

  for (const auto& pSourceVoice : pAudioFile->sourceVoices) {
    // Flags = XAUDIO2_PLAY_TAILS (if using this, don't call FlushSourceBuffers)
    if (FAILED(pSourceVoice->Stop())) {
      OutputDebugStringW(L"ERROR: Stop failed!\n");
      return;
    }

    if (FAILED(pSourceVoice->FlushSourceBuffers())) {
      OutputDebugStringW(L"ERROR: Stop FlushSourceBuffers failed!\n");
      return;
    }
  }

  pAudioFile->isPaused = false;
  pAudioFile->isStopped = true;

  if (pAudioFile->loadType == AudioLoadType::Streaming) {
    pAudioFile->currentStreamBufIndex = 0;
    pAudioFile->ResetToStartPos();
  }
}

void AudioWin::Pause(AudioFileHandle audioFileHandle) {
  AudioFileWin* pAudioFile = (AudioFileWin*)GetAudioFile(audioFileHandle);
  if (!pAudioFile) {
    return;
  }

  if (pAudioFile->isPaused) {
    return;
  }

  // Pause only works for files that only have 1 simultaneous sound
  // (doesn't make sense otherwise)
  // TODO: or does it?? We probably want ability to pause all sounds
  //       (like when pausing the game)
  if (pAudioFile->sourceVoices.size() > 1) {
    return;
  }

  if (!IsPlaying(audioFileHandle)) {
    return;
  }

  for (const auto& pSourceVoice : pAudioFile->sourceVoices) {
    // Flags = XAUDIO2_PLAY_TAILS (if using this, don't call FlushSourceBuffers)
    if (FAILED(pSourceVoice->Stop())) {
      OutputDebugStringW(L"ERROR: Stop failed!\n");
      return;
    }
  }

  pAudioFile->isPaused = true;
}

void AudioWin::Resume(AudioFileHandle audioFileHandle) {
  AudioFileWin* pAudioFile = (AudioFileWin*)GetAudioFile(audioFileHandle);
  if (!pAudioFile) {
    return;
  }

  if (!pAudioFile->isPaused) {
    return;
  }

  // TODO: this is kind of a weird restriction and won't allow MUTE to work! Remove it!!
  // Pause (and Resume) only works for files that only have 1 simultaneous sound
  // (doesn't make sense otherwise)
  if (pAudioFile->sourceVoices.size() > 1) {
    return;
  }

  if (FAILED(pAudioFile->sourceVoices[0]->Start())) {
    OutputDebugStringW(L"ERROR: Resume: Start failed!\n");
    return;
  }

  pAudioFile->isPaused = false;
  pAudioFile->isStopped = false;
}

float AudioWin::GetVolume(AudioFileHandle audioFileHandle) {
  AudioFileWin* pAudioFile = (AudioFileWin*)GetAudioFile(audioFileHandle);
  if (!pAudioFile) {
    return 1.0f;
  }

  // all voices should be at same volume,
  // so return first one.
  if (pAudioFile->sourceVoices.size() > 0) {
    float volume;
    pAudioFile->sourceVoices[0]->GetVolume(&volume);
    return volume;
  }

  return 1.0f;

  // leaving this here for now, for debugging
  // std::vector<float> volumes;
  // for (const auto& pSourceVoice : pAudioFile->sourceVoices)
  //{
  //    float volume;
  //    pSourceVoice->GetVolume(&volume);
  //    volumes.push_back(volume);
  //}

  // return volumes[0];
}

float AudioWin::GetVolume(AudioCategory category) {
  // Calculate the mean iteratively.
  // Algo from: The Art of Computer Programming Vol 2, section 4.2.2

  float avg = 0.0f;
  int x = 1;
  float volume;
  for (const auto& pair : m_fileMap) {
    if (pair.second->category == category) {
      AudioFileWin* pFile = (AudioFileWin*)pair.second;

      // we assume all voices of a sound have the same volume
      if (pFile->sourceVoices.size() > 0) {
        pFile->sourceVoices[0]->GetVolume(&volume);

        avg += (volume - avg) / x;
        ++x;
      }
    }
  }

  return avg;
}

float AudioWin::GetMasterVolume() {
  if (!m_pMasterVoice)
    return 1.0f;

  float volume;
  m_pMasterVoice->GetVolume(&volume);
  return volume;
}

void AudioWin::SetVolume(AudioFileHandle audioFileHandle, float volume) {
  AudioFileWin* pAudioFile = (AudioFileWin*)GetAudioFile(audioFileHandle);
  if (!pAudioFile) {
    return;
  }

  ClampVolume(volume);

  for (const auto& pSourceVoice : pAudioFile->sourceVoices) {
    if (FAILED(pSourceVoice->SetVolume(volume))) {
      OutputDebugStringW(L"ERROR: SetVolume AudioFileHandle: SetVolume failed");
      return;
    }
  }
}

void AudioWin::SetVolume(AudioCategory category, float volume) {
  ClampVolume(volume);

  for (const auto& pair : m_fileMap) {
    if (pair.second->category == category) {
      AudioFileWin* pFile = (AudioFileWin*)pair.second;

      for (const auto& pSourceVoice : pFile->sourceVoices) {
        if (FAILED(pSourceVoice->SetVolume(volume))) {
          OutputDebugStringW(
              L"ERROR: SetVolume AudioCategory: SetVolume failed");
          return;
        }
      }
    }
  }
}

void AudioWin::SetMasterVolume(float& volume) {
  if (!m_pMasterVoice)
    return;

  ClampVolume(volume);

  if (FAILED(m_pMasterVoice->SetVolume(volume))) {
    OutputDebugStringW(L"ERROR: SetMasterVolume: SetVolume failed");
    return;
  }
}

void AudioWin::SetPan(AudioFileHandle audioFileHandle, float pan) {
  AudioFileWin* pAudioFile = (AudioFileWin*)GetAudioFile(audioFileHandle);
  if (!pAudioFile) {
    OutputDebugStringW(L"ERROR: SetPan GetAudioFile failed");
    return;
  }

  // clamp
  if (pan < -1.0f)
    pan = -1.0f;
  else if (pan > 1.0f)
    pan = 1.0f;

  // get speaker config
  DWORD dwChannelMask;
  if (FAILED(m_pMasterVoice->GetChannelMask(&dwChannelMask))) {
    OutputDebugStringW(L"ERROR: SetPan GetChannelMask failed");
    return;
  }

  // 8 channels support up to 7.1 surround
  float outputMatrix[8];
  for (int i = 0; i < 8; i++)
    outputMatrix[i] = 0;

  // linear panning (simple, but not very accurate for center):
  float left = 0.5f - pan / 2;
  float right = 0.5f + pan / 2;

  // channels are always encoded in the order specified
  // on the WAVEFORMATEXTENSIBLE reference page.
  switch (dwChannelMask) {
    case SPEAKER_MONO:
      outputMatrix[0] = 1.0;
      break;
    case SPEAKER_STEREO:
    case SPEAKER_2POINT1:
    case SPEAKER_SURROUND:
      outputMatrix[0] = left;
      outputMatrix[1] = right;
      break;
    case SPEAKER_QUAD:
      outputMatrix[0] = outputMatrix[2] = left;
      outputMatrix[1] = outputMatrix[3] = right;
      break;
    case SPEAKER_4POINT1:
      outputMatrix[0] = outputMatrix[3] = left;
      outputMatrix[1] = outputMatrix[4] = right;
      break;
    case SPEAKER_5POINT1:
    case SPEAKER_7POINT1:
    case SPEAKER_5POINT1_SURROUND:
      outputMatrix[0] = outputMatrix[4] = left;
      outputMatrix[1] = outputMatrix[5] = right;
      break;
    case SPEAKER_7POINT1_SURROUND:
      outputMatrix[0] = outputMatrix[4] = outputMatrix[6] = left;
      outputMatrix[1] = outputMatrix[5] = outputMatrix[7] = right;
      break;
    default:
      OutputDebugStringW(L"ERROR: SetPan Unsupported speaker config");
      return;
  }

  // Apply the output matrix to the originating voice

  XAUDIO2_VOICE_DETAILS masterVoiceDetails;
  m_pMasterVoice->GetVoiceDetails(&masterVoiceDetails);

  XAUDIO2_VOICE_DETAILS voiceDetails;
  for (const auto& pSourceVoice : pAudioFile->sourceVoices) {
    pSourceVoice->GetVoiceDetails(&voiceDetails);

    if (FAILED(pSourceVoice->SetOutputMatrix(
            nullptr, voiceDetails.InputChannels,
            masterVoiceDetails.InputChannels, outputMatrix))) {
      OutputDebugStringW(L"ERROR: SetPan SetOutputMatrix failed");
      return;
    }
  }

  pAudioFile->pan = pan;
}

float AudioWin::GetPan(AudioFileHandle audioFileHandle) {
  AudioFileBase* pAudioFile = GetAudioFile(audioFileHandle);
  if (!pAudioFile) {
    OutputDebugStringW(L"ERROR: GetPan GetAudioFile failed");
    return 0.0f;
  }

  return pAudioFile->pan;
}

bool AudioWin::IsPlaying(AudioFileHandle audioFileHandle) {
  AudioFileWin* pAudioFile = (AudioFileWin*)GetAudioFile(audioFileHandle);
  if (!pAudioFile) {
    OutputDebugStringW(L"ERROR: IsPlaying GetAudioFile failed");
    return false;
  }

  if (pAudioFile->isPaused)
    return false;

  // returns true if at least one voice of this sound is queued up within
  // XAudio2.
  XAUDIO2_VOICE_STATE state;
  for (const auto& pSourceVoice : pAudioFile->sourceVoices) {
    pSourceVoice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
    // BuffersQueued is the number of audio buffers currently queued
    // on the voice, including the one that is processed currently.
    if (state.BuffersQueued > 0)
      return true;
  }

  return false;
}

bool AudioWin::IsPaused(AudioFileHandle audioFileHandle) {
  AudioFileBase* pAudioFile = GetAudioFile(audioFileHandle);
  if (!pAudioFile) {
    OutputDebugStringW(L"ERROR: IsPaused GetAudioFile failed");
    return false;
  }

  return pAudioFile->isPaused;
}

void AudioWin::ClampVolume(float& volume) {
  if (volume < AUDIO_MIN_VOLUME)
    volume = AUDIO_MIN_VOLUME;
  else if (volume > AUDIO_MAX_VOLUME)
    volume = AUDIO_MAX_VOLUME;
}

}  // namespace Mana
