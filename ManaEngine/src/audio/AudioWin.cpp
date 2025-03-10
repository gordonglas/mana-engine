#include "pch.h"
#include <assert.h>
#include "audio/AudioWin.h"
#include "audio/AudioFileBase.h"
#include "audio/AudioFileOggWin.h"
#include "concurrency/IThread.h"

namespace Mana {

AudioBase* g_pAudioEngine = nullptr;

bool AudioWin::Init() {
  if (!com_.IsInitialized()) {
    return false;
  }

  pXAudio2_ = nullptr;
  pMasterVoice_ = nullptr;

  HRESULT hr;
  if (FAILED(hr = XAudio2Create(&pXAudio2_, 0, XAUDIO2_USE_DEFAULT_PROCESSOR)))
    return false;

  if (FAILED(hr = pXAudio2_->CreateMasteringVoice(&pMasterVoice_)))
    return false;

  lastAudioFileHandle_ = 0;

  return true;
}

void AudioWin::Uninit() {
  // stop and destroy all voices and buffers by calling Unload on all AudioFiles

  if (pXAudio2_) {
    pXAudio2_->StopEngine();
  }

  // since Unload calls "fileMap_.erase",
  // and therefore alters the foreach iterator,
  // first form a list of audio handles,
  // then use those to call Unload.
  std::vector<AudioFileHandle> fileHandleList;
  for (auto it = fileMap_.begin(); it != fileMap_.end(); ++it) {
    AudioFileBase* pAudioFile = it->second;
    fileHandleList.push_back(pAudioFile->audioFileHandle_);
  }

  for (size_t i = 0; i < fileHandleList.size(); ++i) {
    Unload(fileHandleList[i]);
  }

  if (pMasterVoice_) {
    pMasterVoice_->DestroyVoice();
    pMasterVoice_ = nullptr;
  }

  if (pXAudio2_) {
    pXAudio2_->Release();
    pXAudio2_ = nullptr;
  }
}

AudioFileHandle AudioWin::Load(const xstring& filePath,
                               AudioCategory category,
                               AudioFormat format,
                               int64_t loopBackPcmSamplePos,
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

  pFile->filePath_ = filePath;
  pFile->category_ = category;
  pFile->format_ = format;
  pFile->loopBackPcmSamplePos_ = loopBackPcmSamplePos;

  if (!pFile->Load(filePath)) {
    delete pFile;
    return 0;
  }

  if (simultaneousSounds > 1 && pFile->loadType_ == AudioLoadType::Streaming) {
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

  pFile->audioFileHandle_ = audioFileHandle;

  fileMap_.insert(std::map<AudioFileHandle, AudioFileBase*>::value_type(
      audioFileHandle, pFile));

  int numSounds = simultaneousSounds;
  while (numSounds > 0) {
    IXAudio2SourceVoice* pSourceVoice = nullptr;

    //if (pFile->loadType == AudioLoadType::Streaming) {
    //  if (FAILED(pXAudio2_->CreateSourceVoice(
    //          &pSourceVoice, (WAVEFORMATEX*)&pFile->wfx_, 0u, 2.0f,
    //          (IXAudio2VoiceCallback*)&voiceCallback))) {
    //    OutputDebugStringW(L"ERROR: CreateSourceVoice streaming failed\n");
    //    delete pFile;
    //    return 0;
    //  }
    //} else {
      if (FAILED(pXAudio2_->CreateSourceVoice(&pSourceVoice,
                                               (WAVEFORMATEX*)&pFile->wfx_))) {
        OutputDebugStringW(L"ERROR: CreateSourceVoice failed\n");
        delete pFile;
        return 0;
      }
    //}

    pFile->sourceVoices_.push_back(pSourceVoice);
    numSounds--;
  }

  // set defaults
  pFile->sourceVoicePos_ = 0;
  pFile->isPaused_ = false;
  pFile->pan_ = 0.0f;

  return audioFileHandle;
}

void AudioWin::Unload(AudioFileHandle audioFileHandle) {
  AudioFileWin* pAudioFile = (AudioFileWin*)GetAudioFile(audioFileHandle);
  if (!pAudioFile) {
    return;
  }

  for (size_t i = 0; i < pAudioFile->sourceVoices_.size(); ++i) {
    auto* pSourceVoice = pAudioFile->sourceVoices_[i];

    // DestroyVoice waits for the XAudio2 audio processing thread to be
    // idle, so it can take a little while (typically no more than a
    // couple of milliseconds). This is necessary to guarantee that the
    // voice will no longer make any callbacks or read any audio data,
    // so the application can safely free up these resources as soon
    // as the call returns.
    pSourceVoice->DestroyVoice();
    pSourceVoice = nullptr;
  }

  fileMap_.erase(pAudioFile->audioFileHandle_);

  pAudioFile->sourceVoices_.clear();

  pAudioFile->Unload();

  delete pAudioFile;
  pAudioFile = nullptr;
}

void AudioWin::Update() {
  XAUDIO2_VOICE_STATE voiceState;
  XAUDIO2_BUFFER buffer;

  GetStreamingFiles();
  for (AudioFileBase* pFileBase : streamingFiles_) {
    AudioFileWin* pFile = static_cast<AudioFileWin*>(pFileBase);

    if (pFile->isStopped_)
      continue;

    if (pFile->lastBufferPlaying_) {
      // last buffer just finished playing.
      //OutputDebugStringW(L"file done playing\n");
      pFile->currentStreamBufIndex_ = 0;
      pFile->StreamSeek(0);
      pFile->isStopped_ = true;
      continue;
    }

    // NOTE: even if this streaming sound is paused,
    // we still want to fill it's buffers and queue them on it's
    // XAudio2-voice. They just won't play until the sound is resumed.

    int bytesPerSample = pFile->wfx_.Format.wBitsPerSample / 8;

    // get number of buffers currently in the XAudio2-Voice queue
    pFile->sourceVoices_[0]->GetState(&voiceState,
                                     XAUDIO2_VOICE_NOSAMPLESPLAYED);

    while (voiceState.BuffersQueued < AudioStreamBufCount) {
      //OutputDebugStringW((std::wstring(L"BuffersQueued: ") +
      //                    std::to_wstring(voiceState.BuffersQueued) + L"\n")
      //                       .c_str());
      //assert(voiceState.BuffersQueued > 0 && "BuffersQueued == 0!");

      // init next buffer with silence
      memset(&pFile->pDataBuffer_[pFile->currentStreamBufIndex_ *
                                  AudioStreamBufSize],
             0, AudioStreamBufSize);

      buffer = {0};

      int readBufLen = AudioStreamBufSize;  // multiple of 4

      size_t destBufPos = pFile->currentStreamBufIndex_ * AudioStreamBufSize;
      unsigned currentBytesRead = 0;
      long actualBytesRead = 1;
      int ovBitstream = 0;

      int bytesToPcmEOF =
          (int)(pFile->totalPcmBytes_ - pFile->currentTotalPcmPos_);
      //OutputDebugStringW((std::wstring(L"Audio Update: bytesToPcmEOF: ") +
      //                    std::to_wstring(bytesToPcmEOF) + L"\n")
      //                       .c_str());
      bool reachedEOF = false;
      if (bytesToPcmEOF < readBufLen) {
        readBufLen = bytesToPcmEOF;
        reachedEOF = true;
        //OutputDebugStringW(L"Audio Update: reachedEOF true\n");
      }

      size_t maxBytesToRead = reachedEOF ? bytesToPcmEOF : AudioStreamBufSize;

      AudioFileOggWin* pOggFile = static_cast<AudioFileOggWin*>(pFile);

      while (actualBytesRead && currentBytesRead < maxBytesToRead) {
        actualBytesRead = ::ov_read(
            &pOggFile->oggVorbisFile_, (char*)&pFile->pDataBuffer_[destBufPos],
            readBufLen, 0, bytesPerSample, 1, &ovBitstream);
        //OutputDebugStringW(
        //    (std::wstring(L"Audio Update: ov_read actualBytesRead: ") +
        //     std::to_wstring(actualBytesRead) + L"\n")
        //        .c_str());
        assert(actualBytesRead >= 0 && "ov_read failed");
        destBufPos += actualBytesRead;
        currentBytesRead += actualBytesRead;
        pFile->currentTotalPcmPos_ += actualBytesRead;
        //OutputDebugStringW((std::wstring(L"Audio Update: ov_read pcmPos: ") +
        //                    std::to_wstring(pFile->currentTotalPcmPos_) + L"\n")
        //                       .c_str());
        if (maxBytesToRead - currentBytesRead < (unsigned)readBufLen) {
          readBufLen = (int)maxBytesToRead - currentBytesRead;
        }
      }

      //bool onLastLoop = false;
      if (reachedEOF && pFile->loopCount_ > 0 &&
          pFile->loopCount_ != AudioBase::LOOP_INFINITE) {
        pFile->loopCount_--;
        //if (pFile->loopCount_ == 0) {
        //  onLastLoop = true;
        //}
      }

      // if reached the end of file and still looping,
      // reset position to start of the file,
      // and fill the rest of the destination buffer.
      if (reachedEOF && pFile->loopCount_ > 0) {
        pFile->StreamSeek(pFile->loopBackPcmSamplePos_);

        if (currentBytesRead < AudioStreamBufSize) {
          // fill the rest of the destination buffer
          readBufLen = AudioStreamBufSize - currentBytesRead;
          actualBytesRead = 1;
          while (actualBytesRead && currentBytesRead < AudioStreamBufSize) {
            actualBytesRead =
                ::ov_read(&pOggFile->oggVorbisFile_,
                          (char*)&pFile->pDataBuffer_[destBufPos], readBufLen,
                          0, bytesPerSample, 1, &ovBitstream);
            //OutputDebugStringW(
            //    (std::wstring(L"Audio Update: ov_read actualBytesRead: ") +
            //     std::to_wstring(actualBytesRead) + L"\n")
            //        .c_str());
            assert(actualBytesRead >= 0 && "ov_read failed");
            destBufPos += actualBytesRead;
            currentBytesRead += actualBytesRead;
            pFile->currentTotalPcmPos_ += actualBytesRead;
            if (AudioStreamBufSize - currentBytesRead < readBufLen) {
              readBufLen = AudioStreamBufSize - currentBytesRead;
            }
          }
        }
      } else {  // not looping (or on last loop)
        if (reachedEOF) {
          pFile->lastBufferPlaying_ = true;
          //OutputDebugStringW(L"Audio Update: set lastBufferPlaying_ true\n");
        }
      }

      buffer.AudioBytes = currentBytesRead;
      buffer.pAudioData = &pFile->pDataBuffer_[pFile->currentStreamBufIndex_ *
                                               AudioStreamBufSize];
      if (pFile->lastBufferPlaying_) {
        buffer.Flags = XAUDIO2_END_OF_STREAM;
      }

      pFile->currentStreamBufIndex_++;
      if (pFile->currentStreamBufIndex_ == AudioStreamBufCount) {
        pFile->currentStreamBufIndex_ = 0;
      }

      HRESULT hr;
      if (FAILED(hr = pFile->sourceVoices_[0]->SubmitSourceBuffer(&buffer))) {
        OutputDebugStringW(L"Audio Update: SubmitSourceBuffer failed\n");
        assert(false && "Audio Update: SubmitSourceBuffer failed");
        return;
      }

      if (pFile->lastBufferPlaying_) {
        break;
      }

      pFile->sourceVoices_[0]->GetState(&voiceState,
                                       XAUDIO2_VOICE_NOSAMPLESPLAYED);
    }
  }
}

bool AudioWin::Play(AudioFileHandle audioFileHandle, uint32_t loopCount) {
  AudioFileWin* pFile = (AudioFileWin*)GetAudioFile(audioFileHandle);
  if (!pFile) {
    return false;
  }

  // if paused, resume
  if (pFile->isPaused_) {
    Resume(audioFileHandle);
    return true;
  }

  // if streaming sound is already playing, do nothing.
  if (pFile->loadType_ == AudioLoadType::Streaming && !pFile->isPaused_ &&
      IsPlaying(audioFileHandle)) {
    return true;
  }

  IXAudio2SourceVoice* pSourceVoice =
      pFile->sourceVoices_[pFile->sourceVoicePos_];

  if (pFile->sourceVoicePos_ == pFile->sourceVoices_.size() - 1) {
    pFile->sourceVoicePos_ = 0;
  } else {
    pFile->sourceVoicePos_++;
  }

  assert(loopCount <= AudioBase::LOOP_INFINITE && "invalid loopCount");
  pFile->loopCount_ = loopCount;

  if (pFile->loadType_ == AudioLoadType::Static) {
    // It's ok for XAUDIO2_BUFFER objects to only be used temporarily.
    // Per docs here:
    // https://docs.microsoft.com/en-us/windows/win32/api/xaudio2/ns-xaudio2-xaudio2_buffer
    // "Memory allocated to hold a XAUDIO2_BUFFER or XAUDIO2_BUFFER_WMA
    // structure
    //  can be freed as soon as the IXAudio2SourceVoice::SubmitSourceBuffer call
    //  it is passed to returns." (except for the underlying data buffer,
    //  which we need for streaming anyway, which is
    //  "AudioFileBase::pDataBuffer_")
    XAUDIO2_BUFFER buffer = {0};
    buffer.AudioBytes =
        (UINT32)pFile->dataBufferSize_;  // Size of the audio buffer in bytes.
    buffer.pAudioData = pFile->pDataBuffer_;  // Buffer containing audio data.
    buffer.Flags = XAUDIO2_END_OF_STREAM;     // Tell the source voice not to
                                           // expect any data after this buffer.

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

    pFile->currentStreamBufIndex_ = 0;
    pFile->StreamSeek(0);
    pFile->lastBufferPlaying_ = false;

    int bytesPerSample = pFile->wfx_.Format.wBitsPerSample / 8;

    size_t bufIndex = 0;
    while (bufIndex < AudioStreamBufCount) {
      memset(&pFile->pDataBuffer_[bufIndex * AudioStreamBufSize], 0,
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
            &pOggFile->oggVorbisFile_, (char*)&pFile->pDataBuffer_[destBufPos],
            readBufLen, 0, bytesPerSample, 1, &ovBitstream);
        //OutputDebugStringW(
        //    (std::wstring(L"ogg file Play: ov_read actualBytesRead: ") +
        //     std::to_wstring(actualBytesRead) + L"\n")
        //        .c_str());
        assert(actualBytesRead >= 0 && "ov_read failed");
        destBufPos += actualBytesRead;
        currentBytesRead += actualBytesRead;
        pFile->currentTotalPcmPos_ += actualBytesRead;
        //OutputDebugStringW(
        //    (std::wstring(L"ogg file Play: ov_read pcmPos: ") +
        //     std::to_wstring(pFile->currentTotalPcmPos_) + L"\n")
        //        .c_str());
        if (AudioStreamBufSize - currentBytesRead < readBufLen) {
          readBufLen = AudioStreamBufSize - currentBytesRead;
        }
      }

      XAUDIO2_BUFFER buffer = {0};
      buffer.AudioBytes = (UINT32)currentBytesRead;
      buffer.pAudioData = &pFile->pDataBuffer_[bufIndex * AudioStreamBufSize];
      buffer.Flags = 0;

      pFile->currentStreamBufIndex_++;
      if (pFile->currentStreamBufIndex_ == AudioStreamBufCount) {
        pFile->currentStreamBufIndex_ = 0;
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

  pFile->isPaused_ = false;
  pFile->isStopped_ = false;

  return true;
}

void AudioWin::Stop(AudioFileHandle audioFileHandle) {
  AudioFileWin* pAudioFile = (AudioFileWin*)GetAudioFile(audioFileHandle);
  if (!pAudioFile || pAudioFile->isStopped_) {
    return;
  }

  for (const auto& pSourceVoice : pAudioFile->sourceVoices_) {
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

  pAudioFile->isPaused_ = false;
  pAudioFile->isStopped_ = true;

  if (pAudioFile->loadType_ == AudioLoadType::Streaming) {
    pAudioFile->currentStreamBufIndex_ = 0;
    pAudioFile->StreamSeek(0);
  }
}

void AudioWin::Pause(AudioFileHandle audioFileHandle) {
  AudioFileWin* pAudioFile = (AudioFileWin*)GetAudioFile(audioFileHandle);
  if (!pAudioFile) {
    return;
  }

  if (pAudioFile->isPaused_) {
    return;
  }

  // Pause only works for files that only have 1 simultaneous sound
  // (doesn't make sense otherwise)
  // TODO: or does it?? We probably want ability to pause all sounds
  //       (like when pausing the game)
  if (pAudioFile->sourceVoices_.size() > 1) {
    return;
  }

  if (!IsPlaying(audioFileHandle)) {
    return;
  }

  for (const auto& pSourceVoice : pAudioFile->sourceVoices_) {
    // Flags = XAUDIO2_PLAY_TAILS (if using this, don't call FlushSourceBuffers)
    if (FAILED(pSourceVoice->Stop())) {
      OutputDebugStringW(L"ERROR: Stop failed!\n");
      return;
    }
  }

  pAudioFile->isPaused_ = true;
}

void AudioWin::Resume(AudioFileHandle audioFileHandle) {
  AudioFileWin* pAudioFile = (AudioFileWin*)GetAudioFile(audioFileHandle);
  if (!pAudioFile) {
    return;
  }

  if (!pAudioFile->isPaused_) {
    return;
  }

  // TODO: this is kind of a weird restriction and won't allow MUTE to work! Remove it!!
  // Pause (and Resume) only works for files that only have 1 simultaneous sound
  // (doesn't make sense otherwise)
  if (pAudioFile->sourceVoices_.size() > 1) {
    return;
  }

  if (FAILED(pAudioFile->sourceVoices_[0]->Start())) {
    OutputDebugStringW(L"ERROR: Resume: Start failed!\n");
    return;
  }

  pAudioFile->isPaused_ = false;
  pAudioFile->isStopped_ = false;
}

float AudioWin::GetVolume(AudioFileHandle audioFileHandle) {
  AudioFileWin* pAudioFile = (AudioFileWin*)GetAudioFile(audioFileHandle);
  if (!pAudioFile) {
    return 1.0f;
  }

  // all voices should be at same volume,
  // so return first one.
  if (pAudioFile->sourceVoices_.size() > 0) {
    float volume;
    pAudioFile->sourceVoices_[0]->GetVolume(&volume);
    return volume;
  }

  return 1.0f;

  // leaving this here for now, for debugging
  // std::vector<float> volumes;
  // for (const auto& pSourceVoice : pAudioFile->sourceVoices_)
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
  for (const auto& pair : fileMap_) {
    if (pair.second->category_ == category) {
      AudioFileWin* pFile = (AudioFileWin*)pair.second;

      // we assume all voices of a sound have the same volume
      if (pFile->sourceVoices_.size() > 0) {
        pFile->sourceVoices_[0]->GetVolume(&volume);

        avg += (volume - avg) / x;
        ++x;
      }
    }
  }

  return avg;
}

float AudioWin::GetMasterVolume() {
  if (!pMasterVoice_)
    return 1.0f;

  float volume;
  pMasterVoice_->GetVolume(&volume);
  return volume;
}

void AudioWin::SetVolume(AudioFileHandle audioFileHandle, float volume) {
  AudioFileWin* pAudioFile = (AudioFileWin*)GetAudioFile(audioFileHandle);
  if (!pAudioFile) {
    return;
  }

  ClampVolume(volume);

  for (const auto& pSourceVoice : pAudioFile->sourceVoices_) {
    if (FAILED(pSourceVoice->SetVolume(volume))) {
      OutputDebugStringW(L"ERROR: SetVolume AudioFileHandle: SetVolume failed");
      return;
    }
  }
}

void AudioWin::SetVolume(AudioCategory category, float volume) {
  ClampVolume(volume);

  for (const auto& pair : fileMap_) {
    if (pair.second->category_ == category) {
      AudioFileWin* pFile = (AudioFileWin*)pair.second;

      for (const auto& pSourceVoice : pFile->sourceVoices_) {
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
  if (!pMasterVoice_)
    return;

  ClampVolume(volume);

  if (FAILED(pMasterVoice_->SetVolume(volume))) {
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
  if (FAILED(pMasterVoice_->GetChannelMask(&dwChannelMask))) {
    OutputDebugStringW(L"ERROR: SetPan GetChannelMask failed");
    return;
  }

  // 8 channels support up to 7.1 surround
  float outputMatrix[8] = {};

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
  pMasterVoice_->GetVoiceDetails(&masterVoiceDetails);

  XAUDIO2_VOICE_DETAILS voiceDetails;
  for (const auto& pSourceVoice : pAudioFile->sourceVoices_) {
    pSourceVoice->GetVoiceDetails(&voiceDetails);

    if (FAILED(pSourceVoice->SetOutputMatrix(
            nullptr, voiceDetails.InputChannels,
            masterVoiceDetails.InputChannels, outputMatrix))) {
      OutputDebugStringW(L"ERROR: SetPan SetOutputMatrix failed");
      return;
    }
  }

  pAudioFile->pan_ = pan;
}

float AudioWin::GetPan(AudioFileHandle audioFileHandle) {
  AudioFileBase* pAudioFile = GetAudioFile(audioFileHandle);
  if (!pAudioFile) {
    OutputDebugStringW(L"ERROR: GetPan GetAudioFile failed");
    return 0.0f;
  }

  return pAudioFile->pan_;
}

bool AudioWin::IsPlaying(AudioFileHandle audioFileHandle) {
  AudioFileWin* pAudioFile = (AudioFileWin*)GetAudioFile(audioFileHandle);
  if (!pAudioFile) {
    OutputDebugStringW(L"ERROR: IsPlaying GetAudioFile failed");
    return false;
  }

  if (pAudioFile->isPaused_)
    return false;

  // returns true if at least one voice of this sound is queued up within
  // XAudio2.
  XAUDIO2_VOICE_STATE state;
  for (const auto& pSourceVoice : pAudioFile->sourceVoices_) {
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

  return pAudioFile->isPaused_;
}

void AudioWin::ClampVolume(float& volume) {
  if (volume < AUDIO_MIN_VOLUME)
    volume = AUDIO_MIN_VOLUME;
  else if (volume > AUDIO_MAX_VOLUME)
    volume = AUDIO_MAX_VOLUME;
}

}  // namespace Mana
