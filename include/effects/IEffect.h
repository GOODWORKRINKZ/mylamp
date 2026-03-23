#pragma once

#include "effects/EffectContext.h"

namespace lamp::effects {

class IEffect {
 public:
  virtual ~IEffect() = default;

  virtual const char* name() const = 0;
  virtual void render(EffectContext& context) = 0;
};

}  // namespace lamp::effects
