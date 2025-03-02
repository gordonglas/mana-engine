#include "pch.h"
#include "audio/AudioFileBase.h"

namespace Mana {

AudioFileBase::AudioFileBase()
    : audioFileHandle_(0),
      category_(AudioCategory::Sound),
      loadType_(AudioLoadType::Static),
      format_(AudioFormat::Wav),
      pan_(0.0f),
      isPaused_(false),
      isStopped_(true),
      fileSize_(0),
      pDataBuffer_(nullptr),
      dataBufferSize_(0),
      currentStreamBufIndex_(0),
      totalPcmBytes_(0),
      currentTotalPcmPos_(0),
      lastBufferPlaying_(false),
      loopCount_(0),
      loopBackPcmSamplePos_(0) {
}

AudioFileBase::~AudioFileBase() {
  if (pDataBuffer_) {
    delete[] pDataBuffer_;
    pDataBuffer_ = nullptr;
  }
}

}  // namespace Mana
