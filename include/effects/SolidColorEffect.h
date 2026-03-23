#pragma once

#include "FrameBuffer.h"
#include "effects/IEffect.h"

namespace lamp::effects {

class SolidColorEffect : public IEffect {
 public:
  explicit SolidColorEffect(Rgb color);

  const char* name() const override;
  void render(EffectContext& context) override;
  void setColor(Rgb color);

 private:
  Rgb color_;
};

}  // namespace lamp::effects
