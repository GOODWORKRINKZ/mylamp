#include "effects/EffectRegistry.h"

#include <cstring>

namespace lamp::effects {

void EffectRegistry::add(IEffect& effect) {
  effects_.push_back(&effect);
  if (active_ == nullptr) {
    active_ = &effect;
  }
}

bool EffectRegistry::setActiveByName(const char* effectName) {
  if (effectName == nullptr) {
    return false;
  }

  for (IEffect* effect : effects_) {
    if (effect != nullptr && std::strcmp(effect->name(), effectName) == 0) {
      active_ = effect;
      return true;
    }
  }

  return false;
}

IEffect* EffectRegistry::active() {
  return active_;
}

const IEffect* EffectRegistry::active() const {
  return active_;
}

size_t EffectRegistry::count() const {
  return effects_.size();
}

void EffectRegistry::renderActive(EffectContext& context) {
  if (active_ != nullptr) {
    active_->render(context);
  }
}

}  // namespace lamp::effects
