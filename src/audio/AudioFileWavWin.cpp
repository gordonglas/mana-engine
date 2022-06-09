#include "pch.h"
#include "audio/AudioFileWavWin.h"
#include "utils/File.h"

// these only work on little-endian systems
#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT ' tmf'
#define fourccWAVE 'EVAW'
#define fourccXWMA 'AMWX'
#define fourccDPDS 'sdpd'

namespace Mana {

AudioFileWavWin::AudioFileWavWin() {}

AudioFileWavWin::~AudioFileWavWin() {}

static HRESULT FindChunk(HANDLE hFile,
                         DWORD fourcc,
                         DWORD& dwChunkSize,
                         DWORD& dwChunkDataPosition) {
  HRESULT hr = S_OK;
  if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
    return HRESULT_FROM_WIN32(GetLastError());

  DWORD dwChunkType;
  DWORD dwChunkDataSize;
  DWORD dwRIFFDataSize = 0;
  DWORD dwFileType;
  DWORD bytesRead = 0;
  DWORD dwOffset = 0;

  while (hr == S_OK) {
    DWORD dwRead;
    if (0 == ReadFile(hFile, &dwChunkType, sizeof(DWORD), &dwRead, NULL))
      hr = HRESULT_FROM_WIN32(GetLastError());

    if (0 == ReadFile(hFile, &dwChunkDataSize, sizeof(DWORD), &dwRead, NULL))
      hr = HRESULT_FROM_WIN32(GetLastError());

    switch (dwChunkType) {
      case fourccRIFF:
        dwRIFFDataSize = dwChunkDataSize;
        dwChunkDataSize = 4;
        if (0 == ReadFile(hFile, &dwFileType, sizeof(DWORD), &dwRead, NULL))
          hr = HRESULT_FROM_WIN32(GetLastError());
        break;

      default:
        if (INVALID_SET_FILE_POINTER ==
            SetFilePointer(hFile, dwChunkDataSize, NULL, FILE_CURRENT)) {
          return HRESULT_FROM_WIN32(GetLastError());
        }
    }

    dwOffset += sizeof(DWORD) * 2;

    if (dwChunkType == fourcc) {
      dwChunkSize = dwChunkDataSize;
      dwChunkDataPosition = dwOffset;
      return S_OK;
    }

    dwOffset += dwChunkDataSize;

    if (bytesRead >= dwRIFFDataSize)
      return S_FALSE;
  }

  return S_OK;
}

static HRESULT ReadChunkData(HANDLE hFile,
                             void* buffer,
                             DWORD buffersize,
                             DWORD bufferoffset) {
  HRESULT hr = S_OK;
  if (INVALID_SET_FILE_POINTER ==
      SetFilePointer(hFile, bufferoffset, NULL, FILE_BEGIN))
    return HRESULT_FROM_WIN32(GetLastError());
  DWORD dwRead;
  if (0 == ReadFile(hFile, buffer, buffersize, &dwRead, NULL))
    hr = HRESULT_FROM_WIN32(GetLastError());
  return hr;
}

bool AudioFileWavWin::Load(const xstring& strFilePath) {
  // only support static loading for wav
  loadType = AudioLoadType::Static;

  HANDLE hFile = CreateFileW(strFilePath.c_str(), GENERIC_READ, FILE_SHARE_READ,
                             NULL, OPEN_EXISTING, 0, NULL);

  if (hFile == INVALID_HANDLE_VALUE) {
    return false;
  }

  if (SetFilePointer(hFile, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
    return false;
  }

  DWORD dwChunkSize;
  DWORD dwChunkPosition;
  // check the file type, should be fourccWAVE or 'XWMA'
  FindChunk(hFile, fourccRIFF, dwChunkSize, dwChunkPosition);
  DWORD filetype;
  ReadChunkData(hFile, &filetype, sizeof(DWORD), dwChunkPosition);
  if (filetype != fourccWAVE) {
    CloseHandle(hFile);
    return false;
  }

  // Locate the 'fmt ' chunk, and copy its contents into a
  // WAVEFORMATEXTENSIBLE structure.
  FindChunk(hFile, fourccFMT, dwChunkSize, dwChunkPosition);
  ReadChunkData(hFile, &wfx, dwChunkSize, dwChunkPosition);

  // wchar_t msg[50];
  // swprintf_s(msg, L"channels: %d\n", wfx.Format.nChannels);
  // OutputDebugStringW(msg);

  // Locate the 'data' chunk, and read its contents into a buffer.
  // fill out the audio data buffer with the contents of the fourccDATA chunk
  FindChunk(hFile, fourccDATA, dwChunkSize, dwChunkPosition);
  pDataBuffer = new BYTE[dwChunkSize];
  dataBufferSize = dwChunkSize;
  ReadChunkData(hFile, pDataBuffer, dwChunkSize, dwChunkPosition);

  CloseHandle(hFile);
  return true;
}

void AudioFileWavWin::Unload() {
}

}  // namespace Mana
