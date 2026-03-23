#pragma once

namespace lamp::sensors {

struct SensorSample {
  bool readOk = false;
  bool available = false;
  float temperatureC = 0.0f;
  float humidityPercent = 0.0f;
};

class ISensorSource {
 public:
  virtual ~ISensorSource() = default;

  virtual SensorSample read() = 0;
};

}  // namespace lamp::sensors