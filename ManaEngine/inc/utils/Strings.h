#pragma once

#include <string>

namespace Mana {

// TODO: These should only "do work" on Windows.
//       Other platforms, they should just maybe "assign" the input string?
//       And do we need "wstring" to instead be some sort of cross-platform
//       type?
std::string Utf16ToUtf8(std::wstring wide);
std::wstring Utf8ToUtf16(std::string utf8);

}  // namespace Mana
