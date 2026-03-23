#include "sensors/ArduinoAht30SensorSource.h"

#include <Wire.h>

#include <cmath>

#include "AppConfig.h"

namespace lamp::sensors {

bool ArduinoAht30SensorSource::beginIfNeeded() {
  if (!beginAttempted_) {
    Wire.begin(lamp::config::kI2cSdaPin, lamp::config::kI2cSclPin);
    initialized_ = sensor_.begin();
    beginAttempted_ = true;
  }

  return initialized_;
}

SensorSample ArduinoAht30SensorSource::read() {
  if (!beginIfNeeded()) {
    return {};
  }

  sensors_event_t humidityEvent;
  sensors_event_t temperatureEvent;
  sensor_.getEvent(&humidityEvent, &temperatureEvent);

  if (std::isnan(temperatureEvent.temperature) ||
      std::isnan(humidityEvent.relative_humidity)) {
    return {};
  }

  SensorSample sample;
  sample.readOk = true;
  sample.available = true;
  sample.temperatureC = temperatureEvent.temperature;
  sample.humidityPercent = humidityEvent.relative_humidity;
  return sample;
}

}  // namespace lamp::sensors