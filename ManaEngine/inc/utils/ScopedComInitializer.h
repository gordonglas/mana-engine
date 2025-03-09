#pragma once

namespace Mana {

// RAII COM initializer
class ScopedComInitializer final {
 public:
  ScopedComInitializer();
  ~ScopedComInitializer();

  bool IsInitialized() const { return initialized_; }

private:
  bool initialized_ = false;
};

}  // namespace Mana
