#include <unity.h>

#include <string>

#include "sensors/ISensorSource.h"
#include "sensors/SensorRuntimeService.h"

namespace {

class FakeSensorSource : public lamp::sensors::ISensorSource {
 public:
  explicit FakeSensorSource(lamp::sensors::SensorSample sample) : sample_(sample) {}

  lamp::sensors::SensorSample read() override {
    return sample_;
  }

 private:
  lamp::sensors::SensorSample sample_;
};

void test_sensor_runtime_reports_fresh_sensor_values() {
  lamp::sensors::SensorRuntimeService service;
  FakeSensorSource source({true, true, 21.75f, 44.5f});

  const lamp::sensors::RuntimeSensorState state = service.refresh({}, source);

  TEST_ASSERT_TRUE(state.available);
  TEST_ASSERT_EQUAL_FLOAT(21.75f, state.temperatureC);
  TEST_ASSERT_EQUAL_FLOAT(44.5f, state.humidityPercent);
  TEST_ASSERT_EQUAL_STRING("Sensor: ok", state.statusLine.c_str());
}

void test_sensor_runtime_marks_sensor_unavailable_without_previous_data() {
  lamp::sensors::SensorRuntimeService service;
  FakeSensorSource source({false, false, 0.0f, 0.0f});

  const lamp::sensors::RuntimeSensorState state = service.refresh({}, source);

  TEST_ASSERT_FALSE(state.available);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, state.temperatureC);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, state.humidityPercent);
  TEST_ASSERT_EQUAL_STRING("Sensor: unavailable", state.statusLine.c_str());
}

void test_sensor_runtime_keeps_last_values_when_sensor_goes_stale() {
  lamp::sensors::SensorRuntimeService service;
  FakeSensorSource source({false, false, 0.0f, 0.0f});
  lamp::sensors::RuntimeSensorState previous;
  previous.available = true;
  previous.temperatureC = 24.0f;
  previous.humidityPercent = 50.0f;
  previous.statusLine = "Sensor: ok";

  const lamp::sensors::RuntimeSensorState state = service.refresh(previous, source);

  TEST_ASSERT_TRUE(state.available);
  TEST_ASSERT_EQUAL_FLOAT(24.0f, state.temperatureC);
  TEST_ASSERT_EQUAL_FLOAT(50.0f, state.humidityPercent);
  TEST_ASSERT_EQUAL_STRING("Sensor: stale", state.statusLine.c_str());
}

void test_sensor_runtime_expires_stale_values_after_repeated_misses() {
  lamp::sensors::SensorRuntimeService service;
  FakeSensorSource source({false, false, 0.0f, 0.0f});
  lamp::sensors::RuntimeSensorState state;
  state.available = true;
  state.temperatureC = 24.0f;
  state.humidityPercent = 50.0f;
  state.statusLine = "Sensor: ok";

  for (int attempt = 0; attempt < 12; ++attempt) {
    state = service.refresh(state, source);
  }

  TEST_ASSERT_FALSE(state.available);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, state.temperatureC);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, state.humidityPercent);
  TEST_ASSERT_EQUAL_STRING("Sensor: unavailable", state.statusLine.c_str());
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_sensor_runtime_reports_fresh_sensor_values);
  RUN_TEST(test_sensor_runtime_marks_sensor_unavailable_without_previous_data);
  RUN_TEST(test_sensor_runtime_keeps_last_values_when_sensor_goes_stale);
  RUN_TEST(test_sensor_runtime_expires_stale_values_after_repeated_misses);
  return UNITY_END();
}