#pragma once

#include <stdint.h>

#include <string>

#include "FrameBuffer.h"

namespace lamp::effects {

class ClockOverlay {
 public:
  void render(const std::string& currentTime, lamp::FrameBuffer& frameBuffer, bool visible) const;

 private:
  void renderDigitColumn(int16_t x, int16_t y, uint8_t digit, lamp::FrameBuffer& frameBuffer) const;
};

}  // namespace lamp::effects