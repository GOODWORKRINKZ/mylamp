#pragma once

#include <stdint.h>

#include "FrameBuffer.h"

namespace lamp::effects {

struct EffectContext {
  uint32_t nowMs;
  FrameBuffer& frameBuffer;
};

}  // namespace lamp::effects
