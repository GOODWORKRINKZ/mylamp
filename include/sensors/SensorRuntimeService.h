#pragma once

#include <string>

#include "sensors/ISensorSource.h"

namespace lamp::sensors {

struct RuntimeSensorState {
  bool available = false;
  float temperatureC = 0.0f;
  float humidityPercent = 0.0f;
  uint8_t consecutiveMisses = 0;
  std::string statusLine;
};

class SensorRuntimeService {
 public:
  RuntimeSensorState refresh(const RuntimeSensorState& previous, ISensorSource& source) const;
};

}  // namespace lamp::sensors