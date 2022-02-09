#include "audio/AudioBase.h"
#include "concurrency/IWorkItem.h"
#include "utils/Lock.h"
#include "utils/StringTypes.h"

namespace Mana {

class WorkItemLoadAudio : public IWorkItem {
 public:
  WorkItemLoadAudio(AudioBase* audioEngine,
                    xstring file,
                    AudioCategory audioCategory,
                    AudioFormat audioFormat,
                    int simultaneousSounds = 1) {
    m_pAudioEngine = audioEngine;
    m_file = file;
    m_audioCategory = audioCategory;
    m_audioFormat = audioFormat;
    m_simultaneousSounds = simultaneousSounds;
    m_handle = 0u;
    m_doneProcessing = false;
  }

  ~WorkItemLoadAudio() override {}

  WorkItemType GetType() override { return WorkItemType::LoadAudio; }

  void Process() override {
    size_t handle = m_pAudioEngine->Load(m_file, m_audioCategory, m_audioFormat,
                                         m_simultaneousSounds);

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
  int m_simultaneousSounds;
  size_t m_handle;
  bool m_doneProcessing;
};

}  // namespace Mana
