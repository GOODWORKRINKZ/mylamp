#pragma once

#include "FrameBuffer.h"
#include "effects/IEffect.h"

namespace lamp::effects {

class UpsideDownEffect : public IEffect {
 public:
  explicit UpsideDownEffect(const char* effectName = "upside-down");

  const char* name() const override;
  void render(EffectContext& context) override;

 private:
  const char* name_;
};

}  // namespace lamp::effects
