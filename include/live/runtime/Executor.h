#pragma once

#include "FrameBuffer.h"
#include "live/runtime/CompiledProgram.h"

namespace lamp::live::runtime {

struct ExecutionContext {
  float timeSeconds = 0.0f;
  float deltaSeconds = 0.0f;
  float temperatureC = 0.0f;
  float humidityPercent = 0.0f;
};

class Executor {
 public:
  void render(const CompiledProgram& program, const ExecutionContext& context,
              lamp::FrameBuffer& frameBuffer) const;
};

}  // namespace lamp::live::runtime