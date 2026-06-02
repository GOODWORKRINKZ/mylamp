#pragma once

#include <stdint.h>

#include <string>

#include "FrameBuffer.h"

namespace lamp::effects {

class ClockOverlay {
 public:
  void render(const std::string& currentTime, lamp::FrameBuffer& frameBuffer,
              bool visible, uint32_t nowMs,
              float temperatureC, float humidityPercent, bool sensorAvailable) const;
};

}  // namespace lamp::effects