#pragma once

namespace Mana {

// RAII COM initializer with separate Init() function,
// so you have control over when it gets initialized.
class ScopedComInitializer final {
 public:
  ScopedComInitializer() = default;
  ~ScopedComInitializer();

  bool Init();
  bool IsInitialized() const { return initialized_; }

private:
  bool initialized_ = false;
};

}  // namespace Mana
