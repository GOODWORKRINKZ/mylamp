#include "sensors/ArduinoAht30SensorSource.h"

#include <Arduino.h>
#include <Wire.h>

#include <cmath>

#include "AppConfig.h"

namespace lamp::sensors {

bool ArduinoAht30SensorSource::beginIfNeeded() {
  if (!beginAttempted_) {
    Wire.begin(lamp::config::kI2cSdaPin, lamp::config::kI2cSclPin);
    Wire.setTimeOut(lamp::config::kI2cTimeoutMs);
    initialized_ = sensor_.begin();
    beginAttempted_ = true;
    if (!initialized_ && !initFailureReported_) {
      Serial.println("AHT30 init failed");
      initFailureReported_ = true;
    }
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

  if (temperatureEvent.temperature < -40.0f || temperatureEvent.temperature > 125.0f ||
      humidityEvent.relative_humidity < 0.0f || humidityEvent.relative_humidity > 100.0f) {
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