#pragma once

#include <Adafruit_AHTX0.h>

#include "sensors/ISensorSource.h"

namespace lamp::sensors {

class ArduinoAht30SensorSource : public ISensorSource {
 public:
  SensorSample read() override;

 private:
  bool beginIfNeeded();

  Adafruit_AHTX0 sensor_;
  bool beginAttempted_ = false;
  bool initialized_ = false;
  bool initFailureReported_ = false;
};

}  // namespace lamp::sensors