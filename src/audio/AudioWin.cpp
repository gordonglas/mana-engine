#include "pch.h"
#include <xaudio2.h>
#include "audio/AudioWin.h"
#include "audio/AudioFileModWin.h"
#include "audio/AudioFileWavWin.h"
#include "concurrency/IThread.h"

namespace Mana {

AudioBase* g_pAudioEngine = nullptr;
IThread* pAudioThread = nullptr;
HANDLE hAudioThreadWait = nullptr;

// TODO: move this somewhere else. (into logger?)
std::wstring GetDateTimeNow() {
  SYSTEMTIME st;
  GetSystemTime(&st);
  char currentTime[84] = "";
  sprintf_s(currentTime, "%.4d%.2d%.2d%.2d%.2d%.2d%.4d", st.wYear, st.wMonth,
            st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
  return std::wstring(Utf8ToUtf16(currentTime));
}

// our own audio thread / AKA the streaming thread
unsigned long AudioThreadFunction(IThread* pThread) {
  hAudioThreadWait = CreateEventW(nullptr, FALSE, FALSE, nullptr);
  if (hAudioThreadWait == nullptr) {
    ManaLogLnError(Channel::Init,
                   _X("AudioThreadFunction: CreateEventW failed"));
    return 0;
  }

  bool lastBufferPlaying = false;
  bool submitBuffer = true;

  XAUDIO2_VOICE_STATE voiceState;
  while (!pThread->IsStopping()) {
    // TODO: probably need to protect most(or all?) accesses of "m_fileMap" with
    // CriticalSection.
    //       maybe change it to a "SynchronizedMap"?
    std::vector<AudioFileBase*> streamingFiles =
        g_pAudioEngine->GetStreamingFiles();

    // fill/queue any empty streaming XAudio buffers
    // See:
    // https://docs.microsoft.com/en-us/windows/win32/xaudio2/how-to--use-source-voice-callbacks
    // and:
    // https://docs.microsoft.com/en-us/windows/win32/xaudio2/how-to--stream-a-sound-from-disk
    // and: https://www.gamedev.net/forums/topic.asp?topic_id=496350

    XAUDIO2_BUFFER buffer;

    for (AudioFileBase* pFileBase : streamingFiles) {
      AudioFileWin* pFile = (AudioFileWin*)pFileBase;

      if (pFile->lastBufferPlaying) {
        // last buffer just finished playing.
        // If looping, fill next buffer with sound,
        // else just don't submit anymore buffers (and maybe stop sound?)

        // for libopenmpt, looping is handled with a call to that lib,
        // so if looping is used with mods, code will never reach here,
        // since libopenmpt will keep filling buffers continually (handles the
        // looping for us).

        // so if looping here, it must be an ogg vorbis stream
        // TODO: handle looping for ogg vorbis.
        // if (pFile->IsLooping)

        OutputDebugStringW(L"file done playing\n");
        continue;
      }

      submitBuffer = true;

      pFile->sourceVoices[0]->GetState(&voiceState,
                                       XAUDIO2_VOICE_NOSAMPLESPLAYED);
      while (voiceState.BuffersQueued < AudioStreamBufCount) {
        OutputDebugStringW((std::wstring(L"BuffersQueued: ") +
                            std::to_wstring(voiceState.BuffersQueued) + L"\n")
                               .c_str());

        // init next buffer with silence (zeros)
        memset(&pFile->pDataBuffer[pFile->currentStreamBufIndex *
                                   AudioStreamBufSize],
               0, AudioStreamBufSize);

        buffer = {0};
        size_t numSamples =
            AudioStreamBufSize / (pFile->wfx.Format.wBitsPerSample / 8);
        size_t pcmFrames = numSamples / pFile->wfx.Format.nChannels;

        if (pFile->format == AudioFormat::Mod) {
          // OutputDebugStringW(L"FILL MOD BUF\n");

          AudioFileModWin* pModFile = (AudioFileModWin*)pFile;

          // TODO: move this to an override'd function in AudioFileModWin, so we keep all the mod load/reads in one place.
          std::size_t pcmFramesRendered =
              pModFile->modLib->read_interleaved_stereo(
                  pModFile->wfx.Format.nSamplesPerSec, pcmFrames,
                  (std::int16_t*)&pModFile
                      ->pDataBuffer[pModFile->currentStreamBufIndex *
                                    AudioStreamBufSize]);

          if (pcmFramesRendered == 0) {
            // end of the file reached. nothing more to render.
            // if we haven't already submitted a buffer with
            // XAUDIO2_END_OF_STREAM, do so now. This handles rare case when
            // final actual buffer of the song ends up being (pcmFramesRendered
            // == pcmFrames).

            if (pFile->lastBufferPlaying) {
              submitBuffer = false;
              OutputDebugStringW(L"LAST BUF ALREADY SUBMITTED 2\n");
            } else {
              pFile->lastBufferPlaying = true;
              lastBufferPlaying = true;
              OutputDebugStringW(L"FINAL BUF 2\n");

              // buffer should be all silence (zeros) at this point.
              // TODO: probably should only pass a small set of the frames
              buffer.AudioBytes = AudioStreamBufSize;
              buffer.pAudioData =
                  &pFile->pDataBuffer[pFile->currentStreamBufIndex *
                                      AudioStreamBufSize];
              buffer.Flags = XAUDIO2_END_OF_STREAM;
            }
          } else {
            if (pFile->lastBufferPlaying) {
              submitBuffer = false;
              OutputDebugStringW(L"LAST BUF ALREADY SUBMITTED\n");
            } else {
              if (pcmFramesRendered < pcmFrames) {
                pFile->lastBufferPlaying = true;
                lastBufferPlaying = true;
                OutputDebugStringW(L"FINAL BUF\n");
              }

              buffer.AudioBytes =
                  (UINT32)pcmFramesRendered * pFile->wfx.Format.nBlockAlign;
              buffer.pAudioData =
                  &pFile->pDataBuffer[pFile->currentStreamBufIndex *
                                      AudioStreamBufSize];
              buffer.Flags = lastBufferPlaying ? XAUDIO2_END_OF_STREAM : 0;
            }
          }
        }

        pFile->currentStreamBufIndex++;
        if (pFile->currentStreamBufIndex == AudioStreamBufCount) {
          pFile->currentStreamBufIndex = 0;
        }
        // TODO: if looping.. this could eventually overflow! fix this.
        pFile->currentBufPos += buffer.AudioBytes;

        if (!submitBuffer) {
          break;
        }

        HRESULT hr;
        if (FAILED(hr = pFile->sourceVoices[0]->SubmitSourceBuffer(&buffer))) {
          return 0;
        }

        if (pFile->lastBufferPlaying) {
          break;
        }

        pFile->sourceVoices[0]->GetState(&voiceState,
                                         XAUDIO2_VOICE_NOSAMPLESPLAYED);
      }
    }

    // Wait function is signaled in the XAudio OnBufferEnd callback or in Uninit
    std::wstring log;
    if (lastBufferPlaying)
      log = GetDateTimeNow() + L": B4 WaitForSingleObjectEx\n";
    DWORD waitRet = WaitForSingleObjectEx(hAudioThreadWait, INFINITE, TRUE);
    if (lastBufferPlaying) {
      log += GetDateTimeNow() + L": AF WaitForSingleObjectEx\n";
      log += L"WaitRet: ";
      log += (waitRet == WAIT_OBJECT_0 ? L"WAIT_OBJECT_0" : L"unknown");
      log += L"\n";
      OutputDebugStringW(log.c_str());
    }
  }

  return 0;
}

class VoiceCallback : public IXAudio2VoiceCallback {
 public:
  VoiceCallback() {}
  ~VoiceCallback() {}

  // All XAudio2 callbacks must complete within 2 milliseconds or less,
  // since they run on the XAudio2 internal audio processing thread,
  // else the buffer won't be filled fast enough and there will be audio issues.

  // called when a voice buffer is freed
  __declspec(nothrow) void __stdcall OnBufferEnd(
      void* pBufferContext) override {
    UNREFERENCED_PARAMETER(pBufferContext);

    // Signal our own audio thread, so it can fill/queue buffers.
    ::SetEvent(hAudioThreadWait);
  }

  // TODO: handle errors. The voice may have to be destroyed and re-created to
  // recover from the error.
  __declspec(nothrow) void __stdcall OnVoiceError(void* pBufferContext,
                                                  HRESULT Error) override {
    UNREFERENCED_PARAMETER(pBufferContext);
    UNREFERENCED_PARAMETER(Error);
  }

  // all other unhandled callbacks are stubs:
  __declspec(nothrow) void __stdcall OnBufferStart(
      void* pBufferContext) override {
    UNREFERENCED_PARAMETER(pBufferContext);
  }
  __declspec(nothrow) void __stdcall OnLoopEnd(void* pBufferContext) override {
    UNREFERENCED_PARAMETER(pBufferContext);
  }
  __declspec(nothrow) void __stdcall OnStreamEnd() override {}
  __declspec(nothrow) void __stdcall OnVoiceProcessingPassEnd() override {}
  __declspec(nothrow) void __stdcall OnVoiceProcessingPassStart(
      UINT32 BytesRequired) override {
    UNREFERENCED_PARAMETER(BytesRequired);
  }
};

// TODO: will a single VoiceCallback object work properly with multiple
// simultaneous streaming files?
VoiceCallback voiceCallback;

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

  // create audio stream update thread
  pAudioThread = Mana::ThreadFactory::Create(AudioThreadFunction);
  pAudioThread->Start();

  return true;
}

void AudioWin::Uninit() {
  // stop and destroy all voices and buffers by calling Unload on all AudioFiles

  if (m_pXAudio2) {
    m_pXAudio2->StopEngine();
  }

  // stop the streaming audio thread
  pAudioThread->Stop();
  ::SetEvent(hAudioThreadWait);
  pAudioThread->Join();
  delete pAudioThread;
  pAudioThread = nullptr;
  ::CloseHandle(hAudioThreadWait);
  hAudioThreadWait = nullptr;

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
  if (format == AudioFormat::Wav) {
    pFile = new AudioFileWavWin;
  }
  else if (format == AudioFormat::Mod) {
    pFile = new AudioFileModWin;
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

    if (pFile->loadType == AudioLoadType::Streaming) {
      if (FAILED(m_pXAudio2->CreateSourceVoice(
              &pSourceVoice, (WAVEFORMATEX*)&pFile->wfx, 0u, 2.0f,
              (IXAudio2VoiceCallback*)&voiceCallback))) {
        OutputDebugStringW(L"ERROR: CreateSourceVoice streaming failed\n");
        delete pFile;
        return 0;
      }
    } else {
      if (FAILED(m_pXAudio2->CreateSourceVoice(&pSourceVoice,
                                               (WAVEFORMATEX*)&pFile->wfx))) {
        OutputDebugStringW(L"ERROR: CreateSourceVoice failed\n");
        delete pFile;
        return 0;
      }

      // note that static sounds don't call SubmitSourceBuffer until "Play" is
      // called, so they can support multiple simultaneous copies.
    }

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

    // DestroyVoice waits for the audio processing thread to be idle,
    // so it can take a little while (typically no more than a couple
    // of milliseconds). This is necessary to guarantee that the voice
    // will no longer make any callbacks or read any audio data,
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

bool AudioWin::Play(AudioFileHandle audioFileHandle) {
  return Play(audioFileHandle, false, 0, 0, 0);
}

bool AudioWin::Play(AudioFileHandle audioFileHandle,
                    unsigned loopCount,
                    unsigned loopBegin,
                    unsigned loopLength) {
  return Play(audioFileHandle, true, loopCount, loopBegin, loopLength);
}

bool AudioWin::Play(AudioFileHandle audioFileHandle,
                    bool updateLoopFields,
                    unsigned loopCount,
                    unsigned loopBegin,
                    unsigned loopLength) {
  AudioFileWin* pFile = (AudioFileWin*)GetAudioFile(audioFileHandle);
  if (!pFile) {
    return false;
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

  // wchar_t buf[80];
  // swprintf(buf, 80, L"sourceVoicePos: [%d]\n", pFile->sourceVoicePos);
  // OutputDebugStringW(buf);

  if (updateLoopFields) {
    if (loopCount == 0) {
      loopBegin = loopLength = 0;
    }

    if (loopCount > MAX_LOOP_COUNT) {
      loopCount = MAX_LOOP_COUNT;
    }

    if (pFile->loadType == AudioLoadType::Streaming) {
      if (pFile->format == AudioFormat::Mod) {
        AudioFileModWin* pModFile = (AudioFileModWin*)pFile;
        // TODO: call AudioFileModWin->SetRepeatCount or something
        pModFile->modLib->set_repeat_count(
            loopCount == LOOP_INFINITE ? -1 : (int32_t)loopCount);
      }
    }
  }

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

    if (updateLoopFields) {
      buffer.LoopBegin = pFile->loopBegin = loopBegin;
      buffer.LoopLength = pFile->loopLength = loopLength;
      buffer.LoopCount = pFile->loopCount = loopCount;
    }

    if (FAILED(pSourceVoice->SubmitSourceBuffer(&buffer))) {
      OutputDebugStringW(L"ERROR: Play SubmitSourceBuffer failed!\n");
      return false;
    }
  } else  // streaming
  {
    // Setup streaming buffers.
    // Since we're using the XAudio2 OnBufferEnd callback, we need to submit
    // more than 1 buffer to prevent a short silence when the first buffer
    // finishes playing.

    size_t numSamples =
        AudioStreamBufSize / (pFile->wfx.Format.wBitsPerSample / 8);
    size_t pcmFrames = numSamples / pFile->wfx.Format.nChannels;

    size_t bufIndex = 0;
    while (bufIndex < AudioStreamBufCount) {
      AudioFileModWin* pModFile = (AudioFileModWin*)pFile;
      // TODO: call same new Read function as needed above (in AudioFileModWin)
      std::size_t pcmFramesRendered =
          pModFile->modLib->read_interleaved_stereo(
            pModFile->wfx.Format.nSamplesPerSec, pcmFrames,
              (std::int16_t*)&pModFile
                  ->pDataBuffer[bufIndex * AudioStreamBufSize]);

      XAUDIO2_BUFFER buffer = {0};
      buffer.AudioBytes =
          (UINT32)pcmFramesRendered * pFile->wfx.Format.nBlockAlign;
      buffer.pAudioData = &pFile->pDataBuffer[bufIndex * AudioStreamBufSize];
      buffer.Flags = 0;

      pFile->currentStreamBufIndex++;
      if (pFile->currentStreamBufIndex == AudioStreamBufCount) {
        pFile->currentStreamBufIndex = 0;
      }
      pFile->currentBufPos += buffer.AudioBytes;

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

  return true;
}

void AudioWin::Stop(AudioFileHandle audioFileHandle) {
  AudioFileWin* pAudioFile = (AudioFileWin*)GetAudioFile(audioFileHandle);
  if (!pAudioFile) {
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
  pAudioFile->currentBufPos = 0;
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

  // Pause (and Resume) only works for files that only have 1 simultaneous sound
  // (doesn't make sense otherwise)
  if (pAudioFile->sourceVoices.size() > 1) {
    return;
  }

  if (!Play(audioFileHandle)) {
    return;
  }
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
