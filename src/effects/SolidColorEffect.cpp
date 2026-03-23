#include "effects/SolidColorEffect.h"

namespace lamp::effects {

SolidColorEffect::SolidColorEffect(Rgb color, const char* effectName)
    : color_(color), name_(effectName) {}

const char* SolidColorEffect::name() const {
  return name_;
}

void SolidColorEffect::render(EffectContext& context) {
  context.frameBuffer.fill(color_);
}

void SolidColorEffect::setColor(Rgb color) {
  color_ = color;
}

}  // namespace lamp::effects
