#include "effects/SolidColorEffect.h"

namespace lamp::effects {

SolidColorEffect::SolidColorEffect(Rgb color) : color_(color) {}

const char* SolidColorEffect::name() const {
  return "solid-color";
}

void SolidColorEffect::render(EffectContext& context) {
  context.frameBuffer.fill(color_);
}

void SolidColorEffect::setColor(Rgb color) {
  color_ = color;
}

}  // namespace lamp::effects
