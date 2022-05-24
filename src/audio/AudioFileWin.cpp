#include "pch.h"
#include "audio/AudioFileWin.h"

namespace Mana {

AudioFileWin::AudioFileWin() : wfx({0}), sourceVoicePos(0) {
  // wfx = {0};
}

AudioFileWin::~AudioFileWin() {}

}  // namespace Mana
