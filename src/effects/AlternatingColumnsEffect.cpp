#include "effects/AlternatingColumnsEffect.h"

#include "AppConfig.h"

namespace lamp::effects {

AlternatingColumnsEffect::AlternatingColumnsEffect(Rgb evenColor, Rgb oddColor,
                                                   const char* effectName)
    : evenColor_(evenColor), oddColor_(oddColor), name_(effectName) {}

const char* AlternatingColumnsEffect::name() const {
  return name_;
}

void AlternatingColumnsEffect::render(EffectContext& context) {
  for (uint8_t y = 0; y < config::kLogicalHeight; ++y) {
    for (uint8_t x = 0; x < config::kLogicalWidth; ++x) {
      context.frameBuffer.setPixel(x, y, (x & 0x01U) == 0 ? evenColor_ : oddColor_);
    }
  }
}

}  // namespace lamp::effects
