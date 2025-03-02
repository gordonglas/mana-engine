#pragma once

#include "audio/AudioBase.h"
#include "concurrency/IWorkItem.h"
#include "concurrency/Mutex.h"
#include "utils/StringTypes.h"

namespace Mana {

class WorkItemLoadAudio : public IWorkItem {
 public:
  WorkItemLoadAudio(AudioBase* audioEngine,
                    const xstring& file,
                    AudioCategory audioCategory,
                    AudioFormat audioFormat,
                    int64_t loopBackPcmSamplePos = 0,
                    int simultaneousSounds = 1) {
    pAudioEngine_ = audioEngine;
    file_ = file;
    audioCategory_ = audioCategory;
    audioFormat_ = audioFormat;
    loopBackPcmSamplePos_ = loopBackPcmSamplePos;
    simultaneousSounds_ = simultaneousSounds;
    handle_ = 0u;
    doneProcessing_ = false;
  }

  virtual ~WorkItemLoadAudio() = default;

  WorkItemLoadAudio(const WorkItemLoadAudio&) = delete;
  WorkItemLoadAudio& operator=(const WorkItemLoadAudio&) = delete;

  WorkItemType GetType() override { return WorkItemType::LoadAudio; }

  void Process() override {
    size_t handle =
        pAudioEngine_->Load(file_, audioCategory_, audioFormat_,
                            loopBackPcmSamplePos_, simultaneousSounds_);

    {
      ScopedMutex lock(lock_);
      handle_ = handle;
      doneProcessing_ = true;
    }
  }

  size_t GetHandleIfDoneProcessing() override {
    ScopedMutex lock(lock_);
    if (doneProcessing_) {
      return handle_;
    } else {
      return 0u;
    }
  }

 private:
  Mutex lock_;
  AudioBase* pAudioEngine_;
  xstring file_;
  AudioCategory audioCategory_;
  AudioFormat audioFormat_;
  int64_t loopBackPcmSamplePos_;
  int simultaneousSounds_;
  size_t handle_;
  bool doneProcessing_;
};

}  // namespace Mana
