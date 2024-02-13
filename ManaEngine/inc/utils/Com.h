#pragma once

namespace Mana {

// use in main thread
class ComInitilizer {
 public:
  static bool Init();
  static void Uninit();

 private:
  static int initRefCount_;
};

}  // namespace Mana
