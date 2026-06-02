#pragma once

#include <stdint.h>

#include <string>

#include "FrameBuffer.h"

namespace lamp::effects {

class ClockOverlay {
 public:
  void render(const std::string& currentTime, lamp::FrameBuffer& frameBuffer, bool visible) const;
};

}  // namespace lamp::effects