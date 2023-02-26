#pragma once

#include "audio/AudioBase.h"
#include "concurrency/IWorkItem.h"
#include "concurrency/Lock.h"
#include "utils/StringTypes.h"

namespace Mana {

class WorkItemLoadAudio : public IWorkItem {
 public:
  WorkItemLoadAudio(AudioBase* audioEngine,
                    xstring file,
                    AudioCategory audioCategory,
                    AudioFormat audioFormat,
                    int64_t loopBackPcmSamplePos = 0,
                    int simultaneousSounds = 1) {
    m_pAudioEngine = audioEngine;
    m_file = file;
    m_audioCategory = audioCategory;
    m_audioFormat = audioFormat;
    m_loopBackPcmSamplePos = loopBackPcmSamplePos;
    m_simultaneousSounds = simultaneousSounds;
    m_handle = 0u;
    m_doneProcessing = false;
  }

  ~WorkItemLoadAudio() override {}

  WorkItemType GetType() override { return WorkItemType::LoadAudio; }

  void Process() override {
    size_t handle =
        m_pAudioEngine->Load(m_file, m_audioCategory, m_audioFormat,
                             m_loopBackPcmSamplePos, m_simultaneousSounds);

    {
      ScopedCriticalSection lock(m_lock);
      m_handle = handle;
      m_doneProcessing = true;
    }
  }

  size_t GetHandleIfDoneProcessing() override {
    ScopedCriticalSection lock(m_lock);
    if (m_doneProcessing) {
      return m_handle;
    } else {
      return 0u;
    }
  }

 private:
  CriticalSection m_lock;
  AudioBase* m_pAudioEngine;
  xstring m_file;
  AudioCategory m_audioCategory;
  AudioFormat m_audioFormat;
  int64_t m_loopBackPcmSamplePos;
  int m_simultaneousSounds;
  size_t m_handle;
  bool m_doneProcessing;
};

}  // namespace Mana
