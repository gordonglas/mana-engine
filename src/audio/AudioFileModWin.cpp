#include "pch.h"
#include "audio/AudioBase.h"
#include "audio/AudioFileModWin.h"
#include "utils/File.h"

#pragma comment(lib, "libopenmpt.lib")

namespace Mana {

AudioFileModWin::AudioFileModWin() : modLib(nullptr) {}

AudioFileModWin::~AudioFileModWin() {}

bool AudioFileModWin::Load(const xstring& strFilePath) {
  // only support streaming loading for mod
  loadType = AudioLoadType::Streaming;

  // load entire mod file into memory
  File* file = new File();
  if (!file) {
    return false;
  }
  fileSize = file->ReadAllBytes(strFilePath.c_str());
  if (fileSize == 0) {
    OutputDebugStringW(L"ReadAllBytes failed");
    delete file;
    return false;
  }

  file->Close();

  modLib = new openmpt::module(file->GetBuffer(), fileSize);
  //modLib = new openmpt::module(file->GetBuffer(), fileSize);
  // don't need the file buffer after module ctor
  delete file;
  file = nullptr;

  // Even though libopenmpt recommends using 32bit float pcm data,
  // XAudio2's optimal format is 16-bit ints according to the XAudio2 docs
  wfx.Format.wFormatTag = WAVE_FORMAT_PCM;
  wfx.Format.nChannels = 2;
  wfx.Format.nSamplesPerSec = 44100UL;  // 48000UL
  wfx.Format.wBitsPerSample = 16;
  wfx.Format.nBlockAlign =
      wfx.Format.nChannels * (wfx.Format.wBitsPerSample / 8);
  wfx.Format.nAvgBytesPerSec =
      wfx.Format.nSamplesPerSec * wfx.Format.nBlockAlign;
  wfx.Format.cbSize = 0;

  pDataBuffer = new BYTE[AudioStreamBufSize * AudioStreamBufCount];

  return true;
}

void AudioFileModWin::Unload() {
  // TODO: deleting this causes a crash??
  //if (modLib) {
  //  delete modLib;
  //  modLib = nullptr;
  //}
}

}  // namespace Mana
