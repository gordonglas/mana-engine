#include "pch.h"
#include "audio/AudioFileBase.h"

namespace Mana {

AudioFileBase::AudioFileBase()
    : audioFileHandle(0),
      category(AudioCategory::Sound),
      loadType(AudioLoadType::Static),
      format(AudioFormat::Wav),
      pan(0.0f),
      isPaused(false),
      isStopped(true),
      pDataBuffer(nullptr),
      dataBufferSize(0),
      loopCount(0),
      loopBackPcmSamplePos(0),
      currentStreamBufIndex(0),
      fileSize(0),
      totalPcmBytes(0),
      currentTotalPcmPos(0),
      lastBufferPlaying(false) {
}

AudioFileBase::~AudioFileBase() {
  if (pDataBuffer) {
    delete[] pDataBuffer;
    pDataBuffer = nullptr;
  }
}

}  // namespace Mana
