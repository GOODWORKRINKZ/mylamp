#pragma once

#include <string>

#include "FrameBuffer.h"
#include "live/runtime/CompiledProgram.h"

namespace lamp {

namespace effects {
class ClockOverlay;
}

namespace live::runtime {

struct ExecutionContext {
  float timeSeconds = 0.0f;
  float deltaSeconds = 0.0f;
  float temperatureC = 0.0f;
  float humidityPercent = 0.0f;
  // Clock overlay integration (Phase 7)
  const lamp::effects::ClockOverlay* clockOverlay = nullptr;
  std::string currentTime;
  bool clockVisible = true;
  uint32_t nowMs = 0;
  bool sensorAvailable = false;
};

class Executor {
 public:
  void render(const CompiledProgram& program, const ExecutionContext& context,
              lamp::FrameBuffer& frameBuffer) const;

 private:
};

}  // namespace live::runtime
}  // namespace lamp