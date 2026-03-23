#include "sensors/SensorRuntimeService.h"

namespace lamp::sensors {

RuntimeSensorState SensorRuntimeService::refresh(const RuntimeSensorState& previous,
                                                 ISensorSource& source) const {
  const SensorSample sample = source.read();
  RuntimeSensorState state = previous;

  if (sample.readOk && sample.available) {
    state.available = true;
    state.temperatureC = sample.temperatureC;
    state.humidityPercent = sample.humidityPercent;
    state.statusLine = "Sensor: ok";
    return state;
  }

  if (previous.available) {
    state.statusLine = "Sensor: stale";
    return state;
  }

  state.available = false;
  state.temperatureC = 0.0f;
  state.humidityPercent = 0.0f;
  state.statusLine = "Sensor: unavailable";
  return state;
}

}  // namespace lamp::sensors