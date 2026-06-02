#pragma once

#include <stddef.h>

#include <vector>

#include "effects/IEffect.h"

namespace lamp::effects {

class EffectRegistry {
 public:
  void add(IEffect& effect);
  bool setActiveByName(const char* effectName);
  IEffect* active();
  const IEffect* active() const;
  size_t count() const;
  void renderActive(EffectContext& context);
  void requestClear();

 private:
  std::vector<IEffect*> effects_;
  IEffect* active_ = nullptr;
  bool clearRequested_ = false;
};

}  // namespace lamp::effects
