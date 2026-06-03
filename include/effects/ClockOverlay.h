#pragma once

#include <stdint.h>

#include <string>

#include "FrameBuffer.h"
#include "live/runtime/CompiledProgram.h"

namespace lamp::effects {

class ClockOverlay {
 public:
  void render(const std::string& currentTime, lamp::FrameBuffer& frameBuffer,
              bool visible, uint32_t nowMs,
              float temperatureC, float humidityPercent, bool sensorAvailable,
              lamp::live::runtime::BlendMode blendMode = lamp::live::runtime::BlendMode::kNormal,
              float alpha = 1.0f) const;
};

}  // namespace lamp::effects