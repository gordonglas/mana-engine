#pragma once

namespace Mana {

// use in main thread
class ComInitializer {
 public:
  ComInitializer() = delete;

  static bool Init();
  static void Uninit();

 private:
  static int initRefCount_;
};

}  // namespace Mana
