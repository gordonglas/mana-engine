#include "pch.h"
#include "audio/audio.h"

// this only works for little-endian (reverse the chars for big-endian systems)
#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT ' tmf'
#define fourccWAVE 'EVAW'
#define fourccXWMA 'AMWX'
#define fourccDPDS 'sdpd'

namespace Mana
{
    AudioFile::AudioFile()
    {
        audioFileHandle = 0;
        category = AudioCategory::Sound;
        loadType = AudioLoadType::Static;
        format = AudioFormat::Wav;
        pan = 0.0f;
        isPaused = false;
        wfx = { 0 };
        pDataBuffer = nullptr;
        buffer = { 0 };
        sourceVoicePos = 0;
    }

    AudioFile::~AudioFile()
    {
        if (pDataBuffer)
        {
            delete[] pDataBuffer;
            pDataBuffer = nullptr;
        }
    }

    static HRESULT FindChunk(HANDLE hFile, DWORD fourcc, DWORD& dwChunkSize, DWORD& dwChunkDataPosition)
    {
        HRESULT hr = S_OK;
        if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
            return HRESULT_FROM_WIN32(GetLastError());

        DWORD dwChunkType;
        DWORD dwChunkDataSize;
        DWORD dwRIFFDataSize = 0;
        DWORD dwFileType;
        DWORD bytesRead = 0;
        DWORD dwOffset = 0;

        while (hr == S_OK)
        {
            DWORD dwRead;
            if (0 == ReadFile(hFile, &dwChunkType, sizeof(DWORD), &dwRead, NULL))
                hr = HRESULT_FROM_WIN32(GetLastError());

            if (0 == ReadFile(hFile, &dwChunkDataSize, sizeof(DWORD), &dwRead, NULL))
                hr = HRESULT_FROM_WIN32(GetLastError());

            switch (dwChunkType)
            {
            case fourccRIFF:
                dwRIFFDataSize = dwChunkDataSize;
                dwChunkDataSize = 4;
                if (0 == ReadFile(hFile, &dwFileType, sizeof(DWORD), &dwRead, NULL))
                    hr = HRESULT_FROM_WIN32(GetLastError());
                break;

            default:
                if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, dwChunkDataSize, NULL, FILE_CURRENT))
                    return HRESULT_FROM_WIN32(GetLastError());
            }

            dwOffset += sizeof(DWORD) * 2;

            if (dwChunkType == fourcc)
            {
                dwChunkSize = dwChunkDataSize;
                dwChunkDataPosition = dwOffset;
                return S_OK;
            }

            dwOffset += dwChunkDataSize;

            if (bytesRead >= dwRIFFDataSize) return S_FALSE;

        }

        return S_OK;
    }

    static HRESULT ReadChunkData(HANDLE hFile, void* buffer, DWORD buffersize, DWORD bufferoffset)
    {
        HRESULT hr = S_OK;
        if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, bufferoffset, NULL, FILE_BEGIN))
            return HRESULT_FROM_WIN32(GetLastError());
        DWORD dwRead;
        if (0 == ReadFile(hFile, buffer, buffersize, &dwRead, NULL))
            hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
    }

    // Requires call to CoInitialize. Call: base/com.h::Init.
    bool Audio::Init()
	{
        m_pXAudio2 = nullptr;
        m_pMasterVoice = nullptr;

		HRESULT hr;
		if (FAILED(hr = XAudio2Create(&m_pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR)))
			return false;

		if (FAILED(hr = m_pXAudio2->CreateMasteringVoice(&m_pMasterVoice)))
			return false;

        m_lastAudioFileHandle = 0;

		return true;
	}

    void Audio::Uninit()
    {
        // stop and destroy all voices by calling Unload on all AudioFiles

        for (auto it = m_fileMap.begin(); it != m_fileMap.end(); ++it)
        {
            AudioFile* pAudioFile = it->second;

            Unload(pAudioFile->audioFileHandle);

            delete pAudioFile;
            pAudioFile = nullptr;
        }

        m_fileMap.clear();
    }

    AudioFileHandle Audio::GetNextFreeAudioFileHandle()
    {
        AudioFileHandle next = m_lastAudioFileHandle + 1;
        if (next > MAX_SOUNDS_LOADED)
            next = 1; // wrap around

        // make sure next value isn't in use
        bool firstPass = true;
        while (true)
        {
            if (m_fileMap.find(next) == m_fileMap.end())
                break;

            next++;
            if (next > MAX_SOUNDS_LOADED) {
                if (!firstPass)
                {
                    return 0; // too many sounds loaded.
                }

                firstPass = false;
                next = 1;
            }
        }

        m_lastAudioFileHandle = next;
        return next;
    }

    AudioFileHandle Audio::Load(std::wstring filePath,
        AudioCategory category, AudioLoadType loadType,
        AudioFormat format, int simultaneousSounds)
	{
        AudioFileHandle audioFileHandle = 0;

        if (simultaneousSounds < 1)
            return 0;

        AudioFile* pFile = new AudioFile;
        if (!pFile)
            return 0;

        pFile->filePath = filePath;
        pFile->category = category;
        pFile->loadType = loadType;
        pFile->format = format;

        if (format == AudioFormat::Wav)
        {
            // only support static loading for wav
            if (loadType == AudioLoadType::Streaming)
            {
                delete pFile;
                return 0;
            }

            HANDLE hFile = CreateFileW(
                filePath.c_str(),
                GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                0,
                NULL);

            if (hFile == INVALID_HANDLE_VALUE)
            {
                delete pFile;
                return 0;
            }

            if (SetFilePointer(hFile, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
            {
                delete pFile;
                return 0;
            }

            DWORD dwChunkSize;
            DWORD dwChunkPosition;
            // check the file type, should be fourccWAVE or 'XWMA'
            FindChunk(hFile, fourccRIFF, dwChunkSize, dwChunkPosition);
            DWORD filetype;
            ReadChunkData(hFile, &filetype, sizeof(DWORD), dwChunkPosition);
            if (filetype != fourccWAVE)
            {
                CloseHandle(hFile);
                delete pFile;
                return false;
            }

            pFile->wfx = { 0 };
            pFile->buffer = { 0 };

            // Locate the 'fmt ' chunk, and copy its contents into a WAVEFORMATEXTENSIBLE structure.
            FindChunk(hFile, fourccFMT, dwChunkSize, dwChunkPosition);
            ReadChunkData(hFile, &pFile->wfx, dwChunkSize, dwChunkPosition);

            //wchar_t msg[50];
            //swprintf_s(msg, L"channels: %d\n", pFile->wfx.Format.nChannels);
            //OutputDebugStringW(msg);

            // Locate the 'data' chunk, and read its contents into a buffer.
            // fill out the audio data buffer with the contents of the fourccDATA chunk
            FindChunk(hFile, fourccDATA, dwChunkSize, dwChunkPosition);
            pFile->pDataBuffer = new BYTE[dwChunkSize];
            ReadChunkData(hFile, pFile->pDataBuffer, dwChunkSize, dwChunkPosition);

            CloseHandle(hFile);

            // Populate an XAUDIO2_BUFFER structure.
            pFile->buffer.AudioBytes = dwChunkSize;  //size of the audio buffer in bytes
            pFile->buffer.pAudioData = pFile->pDataBuffer;  //buffer containing audio data
            pFile->buffer.Flags = XAUDIO2_END_OF_STREAM; // tell the source voice not to expect any data after this buffer
        }
        // TODO: add other formats
        else
        {
            return 0;
        }

        audioFileHandle = GetNextFreeAudioFileHandle();
        if (!audioFileHandle)
        {
            if (pFile)
            {
                if (pFile->pDataBuffer)
                    delete[] pFile->pDataBuffer;
                delete pFile;
            }
            return 0;
        }

        pFile->audioFileHandle = audioFileHandle;

        m_fileMap.insert(std::map<AudioFileHandle, AudioFile*>::value_type(audioFileHandle, pFile));

        int numSounds = simultaneousSounds;
        while (numSounds > 0)
        {
            IXAudio2SourceVoice* pSourceVoice = nullptr;
            if (FAILED(m_pXAudio2->CreateSourceVoice(&pSourceVoice,
                (WAVEFORMATEX*)&pFile->wfx)))
            {
                OutputDebugStringW(L"ERROR: CreateSourceVoice failed\n");
                return false;
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

    void Audio::Unload(AudioFileHandle audioFileHandle)
    {
        AudioFile* pAudioFile = GetAudioFile(audioFileHandle);
        if (!pAudioFile)
        {
            return;
        }

        for (size_t i = 0; i < pAudioFile->sourceVoices.size(); ++i)
        {
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

        pAudioFile->sourceVoices.clear();
    }

    bool Audio::Play(AudioFileHandle audioFileHandle)
    {
        return Play(audioFileHandle, false, 0, 0, 0);
    }

    bool Audio::Play(AudioFileHandle audioFileHandle,
        unsigned loopCount, unsigned loopBegin, unsigned loopLength)
    {
        return Play(audioFileHandle, true, loopCount, loopBegin, loopLength);
    }

    bool Audio::Play(AudioFileHandle audioFileHandle, bool updateLoopFields,
        unsigned loopCount, unsigned loopBegin, unsigned loopLength)
    {
        AudioFile* pAudioFile = GetAudioFile(audioFileHandle);
        if (!pAudioFile)
        {
            return false;
        }

        IXAudio2SourceVoice* pSourceVoice = pAudioFile->sourceVoices[pAudioFile->sourceVoicePos];

        if (pAudioFile->sourceVoicePos == pAudioFile->sourceVoices.size() - 1)
        {
            pAudioFile->sourceVoicePos = 0;
        }
        else
        {
            pAudioFile->sourceVoicePos++;
        }

        //wchar_t buf[80];
        //swprintf(buf, 80, L"sourceVoicePos: [%d]\n", pAudioFile->sourceVoicePos);
        //OutputDebugStringW(buf);

        if (updateLoopFields)
        {
            if (loopCount == 0)
            {
                loopBegin = loopLength = 0;
            }

            if (loopCount > MAX_LOOP_COUNT)
            {
                loopCount = MAX_LOOP_COUNT;
            }

            pAudioFile->buffer.LoopBegin = loopBegin;
            pAudioFile->buffer.LoopLength = loopLength;
            pAudioFile->buffer.LoopCount = loopCount;
        }

        if (!pAudioFile->isPaused)
        {
            if (FAILED(pSourceVoice->SubmitSourceBuffer(&pAudioFile->buffer)))
            {
                OutputDebugStringW(L"ERROR: PlayWav SubmitSourceBuffer failed!\n");
                return false;
            }
        }

        if (FAILED(pSourceVoice->Start()))
        {
            OutputDebugStringW(L"ERROR: PlayWav Start failed!\n");
            return false;
        }

        pAudioFile->isPaused = false;

        return true;
    }

    void Audio::Stop(AudioFileHandle audioFileHandle)
    {
        AudioFile* pAudioFile = GetAudioFile(audioFileHandle);
        if (!pAudioFile)
        {
            return;
        }

        for (const auto& pSourceVoice : pAudioFile->sourceVoices)
        {
            // Flags = XAUDIO2_PLAY_TAILS (if using this, don't call FlushSourceBuffers)
            if (FAILED(pSourceVoice->Stop()))
            {
                OutputDebugStringW(L"ERROR: Stop failed!\n");
                return;
            }

            if (FAILED(pSourceVoice->FlushSourceBuffers()))
            {
                OutputDebugStringW(L"ERROR: Stop FlushSourceBuffers failed!\n");
                return;
            }
        }

        pAudioFile->isPaused = false;
    }

    void Audio::StopAll()
    {
        for (const auto& pair : m_fileMap)
        {
            Stop(pair.first);
        }
    }

    void Audio::Pause(AudioFileHandle audioFileHandle)
    {
        AudioFile* pAudioFile = GetAudioFile(audioFileHandle);
        if (!pAudioFile)
        {
            return;
        }

        if (pAudioFile->isPaused)
        {
            return;
        }

        // Pause only works for files that only have 1 simultaneous sound
        // (doesn't make sense otherwise)
        if (pAudioFile->sourceVoices.size() > 1)
        {
            return;
        }

        if (!IsPlaying(audioFileHandle))
        {
            return;
        }

        for (const auto& pSourceVoice : pAudioFile->sourceVoices)
        {
            // Flags = XAUDIO2_PLAY_TAILS (if using this, don't call FlushSourceBuffers)
            if (FAILED(pSourceVoice->Stop()))
            {
                OutputDebugStringW(L"ERROR: Stop failed!\n");
                return;
            }
        }

        pAudioFile->isPaused = true;
    }

    void Audio::PauseAll()
    {
        for (const auto& pair : m_fileMap)
        {
            Pause(pair.first);
        }
    }

    void Audio::Resume(AudioFileHandle audioFileHandle)
    {
        AudioFile* pAudioFile = GetAudioFile(audioFileHandle);
        if (!pAudioFile)
        {
            return;
        }

        if (!pAudioFile->isPaused)
        {
            return;
        }

        // Pause (and Resume) only works for files that only have 1 simultaneous sound
        // (doesn't make sense otherwise)
        if (pAudioFile->sourceVoices.size() > 1)
        {
            return;
        }

        if (!Play(audioFileHandle))
        {
            return;
        }
    }

    void Audio::ResumeAll()
    {
        for (const auto& pair : m_fileMap)
        {
            Resume(pair.first);
        }
    }

    float Audio::GetVolume(AudioFileHandle audioFileHandle)
    {
        AudioFile* pAudioFile = GetAudioFile(audioFileHandle);
        if (!pAudioFile)
        {
            return 1.0f;
        }

        // all voices should be at same volume,
        // so return first one.
        if (pAudioFile->sourceVoices.size() > 0)
        {
            float volume;
            pAudioFile->sourceVoices[0]->GetVolume(&volume);
            return volume;
        }

        return 1.0f;

        // leaving this here for now, for debugging
        //std::vector<float> volumes;
        //for (const auto& pSourceVoice : pAudioFile->sourceVoices)
        //{
        //    float volume;
        //    pSourceVoice->GetVolume(&volume);
        //    volumes.push_back(volume);
        //}
        
        //return volumes[0];
    }

    float Audio::GetVolume(AudioCategory category)
    {
        // Calculate the mean iteratively.
        // Algo from: The Art of Computer Programming Vol 2, section 4.2.2

        float avg = 0.0f;
        int x = 1;
        float volume;
        for (const auto& pair : m_fileMap)
        {
            if (pair.second->category == category)
            {
                // we assume all voices of a sound have the same volume
                if (pair.second->sourceVoices.size() > 0)
                {
                    pair.second->sourceVoices[0]->GetVolume(&volume);

                    avg += (volume - avg) / x;
                    ++x;
                }
            }
        }

        return avg;
    }

    float Audio::GetMasterVolume()
    {
        if (!m_pMasterVoice)
            return 1.0f;

        float volume;
        m_pMasterVoice->GetVolume(&volume);
        return volume;
    }

    void Audio::SetVolume(AudioFileHandle audioFileHandle, float volume)
    {
        AudioFile* pAudioFile = GetAudioFile(audioFileHandle);
        if (!pAudioFile)
        {
            return;
        }

        ClampVolume(volume);

        for (const auto& pSourceVoice : pAudioFile->sourceVoices)
        {
            if (FAILED(pSourceVoice->SetVolume(volume)))
            {
                OutputDebugStringW(L"ERROR: SetVolume AudioFileHandle: SetVolume failed");
                return;
            }
        }
    }

    void Audio::SetVolume(AudioCategory category, float volume)
    {
        ClampVolume(volume);

        for (const auto& pair : m_fileMap)
        {
            if (pair.second->category == category)
            {
                for (const auto& pSourceVoice : pair.second->sourceVoices)
                {
                    if (FAILED(pSourceVoice->SetVolume(volume)))
                    {
                        OutputDebugStringW(L"ERROR: SetVolume AudioCategory: SetVolume failed");
                        return;
                    }
                }
            }
        }
    }

    void Audio::SetMasterVolume(float& volume)
    {
        if (!m_pMasterVoice)
            return;

        ClampVolume(volume);

        if (FAILED(m_pMasterVoice->SetVolume(volume)))
        {
            OutputDebugStringW(L"ERROR: SetMasterVolume: SetVolume failed");
            return;
        }
    }

    void Audio::SetPan(AudioFileHandle audioFileHandle, float pan)
    {
        AudioFile* pAudioFile = GetAudioFile(audioFileHandle);
        if (!pAudioFile)
        {
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
        if (FAILED(m_pMasterVoice->GetChannelMask(&dwChannelMask)))
        {
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
        switch (dwChannelMask)
        {
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
        for (const auto& pSourceVoice : pAudioFile->sourceVoices)
        {
            pSourceVoice->GetVoiceDetails(&voiceDetails);

            if (FAILED(pSourceVoice->SetOutputMatrix(nullptr, voiceDetails.InputChannels, masterVoiceDetails.InputChannels, outputMatrix)))
            {
                OutputDebugStringW(L"ERROR: SetPan SetOutputMatrix failed");
                return;
            }
        }

        pAudioFile->pan = pan;
    }

    float Audio::GetPan(AudioFileHandle audioFileHandle)
    {
        AudioFile* pAudioFile = GetAudioFile(audioFileHandle);
        if (!pAudioFile)
        {
            OutputDebugStringW(L"ERROR: GetPan GetAudioFile failed");
            return 0.0f;
        }

        return pAudioFile->pan;
    }

    bool Audio::IsPlaying(AudioFileHandle audioFileHandle)
    {
        AudioFile* pAudioFile = GetAudioFile(audioFileHandle);
        if (!pAudioFile)
        {
            OutputDebugStringW(L"ERROR: IsPlaying GetAudioFile failed");
            return false;
        }

        // returns true if at least one voice of this sound is playing
        XAUDIO2_VOICE_STATE state;
        for (const auto& pSourceVoice : pAudioFile->sourceVoices)
        {
            pSourceVoice->GetState(&state, XAUDIO2_VOICE_NOSAMPLESPLAYED);
            // BuffersQueued is the number of audio buffers currently queued
            // on the voice, including the one that is processed currently.
            if (state.BuffersQueued > 0)
                return true;
        }

        return false;
    }

    bool Audio::IsPaused(AudioFileHandle audioFileHandle)
    {
        AudioFile* pAudioFile = GetAudioFile(audioFileHandle);
        if (!pAudioFile)
        {
            OutputDebugStringW(L"ERROR: IsPaused GetAudioFile failed");
            return false;
        }

        return pAudioFile->isPaused;
    }

    // private

    AudioFile* Audio::GetAudioFile(AudioFileHandle audioFileHandle)
    {
        auto search = m_fileMap.find(audioFileHandle);
        if (search == m_fileMap.end())
        {
            return nullptr;
        }

        return search->second;
    }

    void Audio::ClampVolume(float& volume)
    {
        if (volume < AUDIO_MIN_VOLUME)
            volume = AUDIO_MIN_VOLUME;
        else if (volume > AUDIO_MAX_VOLUME)
            volume = AUDIO_MAX_VOLUME;
    }
}
