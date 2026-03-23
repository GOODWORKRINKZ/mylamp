#pragma once

#include "FrameBuffer.h"
#include "effects/IEffect.h"

namespace lamp::effects {

class AlternatingColumnsEffect : public IEffect {
 public:
  AlternatingColumnsEffect(Rgb evenColor, Rgb oddColor,
                           const char* effectName = "alternating-columns");

  const char* name() const override;
  void render(EffectContext& context) override;

 private:
  Rgb evenColor_;
  Rgb oddColor_;
  const char* name_;
};

}  // namespace lamp::effects
