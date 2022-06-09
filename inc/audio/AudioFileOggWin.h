// Windows-specific Mod file format details

#pragma once

#include "ManaGlobals.h"
#include "audio/AudioFileWin.h"
#include "target/TargetOS.h"
#include "utils/File.h"

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

namespace Mana {

class AudioFileOggWin : public AudioFileWin {
 public:
  AudioFileOggWin();
  // dtor doesn't stop sounds or destroy xaudio2 buffers.
  // The Audio engine handles this.
  virtual ~AudioFileOggWin();

  bool Load(const xstring& strFilePath) override;
  void Unload() override;

  void ResetToStartPos() override;

  File* pCompressedOggFile_;
  size_t dataReadSoFar_;     // compressed data pos

  // The struct that's initialized in ov_open_callbacks,
  // then passed to all other libvorbisfile functions.
  OggVorbis_File oggVorbisFile_;
  bool oggVorbisFileLoaded_;
};

// Ogg Vorbis Callbacks, will be called by the Ogg Vorbis lib.
// https://xiph.org/vorbis/doc/vorbisfile/ov_callbacks.html
// https://xiph.org/vorbis/doc/vorbisfile/callbacks.html
// These are necessary to be able to use an in-memory stream
// with the Ogg Vorbis lib. If we were streaming an ogg file
// directly from a hard-drive, we wouldn't need these.

size_t OggVorbisRead(void* pDestData,
                     size_t byteSize,
                     size_t sizeToRead,
                     void* dataSource);

int OggVorbisSeek(void* dataSource, ogg_int64_t offset, int origin);
long OggVorbisTell(void* dataSource);

}  // namespace Mana
