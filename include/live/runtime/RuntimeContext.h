#pragma once

#include <stdint.h>

namespace lamp::live::runtime {

struct RuntimeContext {
  uint32_t nowMs = 0;
  uint32_t deltaMs = 0;
  float temperatureC = 0.0f;
  float humidityPercent = 0.0f;
};

}  // namespace lamp::live::runtime