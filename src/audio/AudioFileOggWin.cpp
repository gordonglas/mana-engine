#include "pch.h"
#include <assert.h>
#include <cerrno>
#include "audio/AudioBase.h"
#include "audio/AudioFileOggWin.h"

// These are dynamic (not static) libs,
// so dlls are required at runtime.
// These dynamic libs are specified via the
// ManaEngine project setting's library files search path.
#pragma comment(lib, "libogg.lib")
#pragma comment(lib, "libvorbis.lib")
#pragma comment(lib, "libvorbisfile.lib")

namespace Mana {

AudioFileOggWin::AudioFileOggWin()
    : pCompressedOggFile_(nullptr),
      dataReadSoFar_(0),
      oggVorbisFile_({0}),
      oggVorbisFileLoaded_(false) {}

AudioFileOggWin::~AudioFileOggWin() {
  Unload();
}

bool AudioFileOggWin::Load(const xstring& strFilePath) {
  // only support streaming loading for ogg
  loadType = AudioLoadType::Streaming;

  // Load entire ogg file into memory.
  // At runtime, we'll decode from memory.
  pCompressedOggFile_ = new File();
  if (!pCompressedOggFile_) {
    return false;
  }
  fileSize = pCompressedOggFile_->ReadAllBytes(strFilePath.c_str());
  if (fileSize == 0) {
    delete pCompressedOggFile_;
    pCompressedOggFile_ = nullptr;
    return false;
  }

  ov_callbacks oggCallbacks;
  oggCallbacks.read_func = OggVorbisRead;
  oggCallbacks.seek_func = OggVorbisSeek;
  // since we're streaming from memory, we don't need a close callback.
  oggCallbacks.close_func = nullptr;
  oggCallbacks.tell_func = OggVorbisTell;

  // ov_open_callbacks makes calls to our ov_callbacks to read in the ogg
  // file's header data
  int ovRet =
      ::ov_open_callbacks(this, &oggVorbisFile_, nullptr, 0, oggCallbacks);
  assert(ovRet >= 0);

  oggVorbisFileLoaded_ = true;

  // get sound format info
  vorbis_info* vi = ::ov_info(&oggVorbisFile_, -1);
  assert(vi != nullptr);

  ::memset(&wfx, 0, sizeof(wfx));
  wfx.Format.wFormatTag = WAVE_FORMAT_PCM;
  wfx.Format.nChannels = (WORD)vi->channels;
  wfx.Format.nSamplesPerSec = vi->rate;
  wfx.Format.wBitsPerSample = 16; // ogg vorbis is always 16 bit
  wfx.Format.nBlockAlign =
      wfx.Format.nChannels * (wfx.Format.wBitsPerSample / 8);
  wfx.Format.nAvgBytesPerSec =
      wfx.Format.nSamplesPerSec * wfx.Format.nBlockAlign;
  wfx.Format.cbSize = 0;

  pDataBuffer = new BYTE[AudioStreamBufSize * AudioStreamBufCount];

  // must seek back to the start after calling ov_open_callbacks
  // and before calling ov_pcm_total!
  ResetToStartPos();

  // Make sure the ogg data size is longer than the streaming buffers size.
  // And get total pcm sample byte size, so we can use it to tell when
  // EOF is reached during streaming later.
  long totalPcmSamples = 0;
  for (int i = 0; i < ::ov_streams(&oggVorbisFile_); ++i) {
    totalPcmSamples += (long)::ov_pcm_total(&oggVorbisFile_, i);
  }
  //long totalPcmSamples = (long)::ov_pcm_total(&oggVorbisFile_, -1);
  OutputDebugStringW((std::wstring(L"ogg file Load: totalPcmSamples: ") +
                      std::to_wstring(totalPcmSamples) + L"\n")
                         .c_str());
  assert(totalPcmSamples > 0);
  long nTotalPcmBytes =
      totalPcmSamples * (wfx.Format.wBitsPerSample / 8) * wfx.Format.nChannels;
  OutputDebugStringW((std::wstring(L"ogg file Load: totalPcmBytes: ") +
                      std::to_wstring(nTotalPcmBytes) + L"\n")
                         .c_str());
  assert((nTotalPcmBytes > AudioStreamBufCount * AudioStreamBufSize) &&
         "ogg file's pcm data must be larger than the streaming buffers "
         "total size! Use a static wav file instead.");
  totalPcmBytes = nTotalPcmBytes;

  // TODO: might also be useful to get the total time so it can be
  //       displayed in an in-game music player along with ability to seek.
  //lenMillis = 1000.f * ov_time_total(&oggVorbisFile_, -1);
  // Also see "ov_time_tell" to get current time offset.

  return true;
}

void AudioFileOggWin::Unload() {
  if (oggVorbisFileLoaded_) {
    ::ov_clear(&oggVorbisFile_);
    oggVorbisFileLoaded_ = false;
  }

  if (pCompressedOggFile_) {
    delete pCompressedOggFile_;
    pCompressedOggFile_ = nullptr;
  }
}

void AudioFileOggWin::ResetToStartPos() {
  currentTotalPcmPos = 0;
  currentStreamBufIndex = 0;

  int seekRet = ::ov_raw_seek(&oggVorbisFile_, 0);
  OutputDebugStringW((std::wstring(L"AudioFileOggWin: ov_raw_seek returned: ") +
                      std::to_wstring(seekRet) + L"\n")
                         .c_str());
  assert(seekRet == 0 && "AudioFileOggWin: ov_raw_seek returned != 0!");
}

size_t OggVorbisRead(void* pDestData,
                     size_t byteSize,
                     size_t sizeToRead,
                     void* dataSource) {
  AudioFileOggWin* pOggFile = static_cast<AudioFileOggWin*>(dataSource);
  if (!pOggFile) {
    // according to vorbis docs,
    // we're supposed to return 0 and set errno to non-zero.
    errno = EFAULT;
    return 0;
  }

  size_t actualSizeToRead;
  size_t bytesToEOF =
      pOggFile->pCompressedOggFile_->GetFileSize() - pOggFile->dataReadSoFar_;
  if ((sizeToRead * byteSize) < bytesToEOF) {
    actualSizeToRead = sizeToRead * byteSize;
  } else {
    actualSizeToRead = bytesToEOF;
  }

  if (actualSizeToRead) {
    ::memcpy(pDestData,
             (char*)pOggFile->pCompressedOggFile_->GetBuffer() +
                 pOggFile->dataReadSoFar_,
             actualSizeToRead);

    pOggFile->dataReadSoFar_ += actualSizeToRead;
  }

  return actualSizeToRead;
}

int OggVorbisSeek(void* dataSource, ogg_int64_t offset, int origin) {
  AudioFileOggWin* pOggFile = static_cast<AudioFileOggWin*>(dataSource);
  if (!pOggFile) {
    return -1L;
  }

  switch (origin) {
    case SEEK_SET: {
      ogg_int64_t actualOffset =
          pOggFile->pCompressedOggFile_->GetFileSize() >= (size_t)offset
              ? offset
              : pOggFile->pCompressedOggFile_->GetFileSize();

      pOggFile->dataReadSoFar_ = static_cast<size_t>(actualOffset);
      break;
    }
    case SEEK_CUR: {
      size_t bytesToEOF = pOggFile->pCompressedOggFile_->GetFileSize() -
                          pOggFile->dataReadSoFar_;

      ogg_int64_t actualOffset =
          (size_t)offset < bytesToEOF ? offset : bytesToEOF;

      pOggFile->dataReadSoFar_ += static_cast<size_t>(actualOffset);
      break;
    }
    case SEEK_END: {
      pOggFile->dataReadSoFar_ =
          pOggFile->pCompressedOggFile_->GetFileSize() + 1;
      break;
    }
    default:
      assert(false && "Bad param 'origin', requires same values as fseek.");
      break;
  }

  return 0;
}

long OggVorbisTell(void* dataSource) {
  AudioFileOggWin* pOggFile = static_cast<AudioFileOggWin*>(dataSource);
  if (!pOggFile) {
    return -1L;
  }

  return static_cast<long>(pOggFile->dataReadSoFar_);
}

}  // namespace Mana
