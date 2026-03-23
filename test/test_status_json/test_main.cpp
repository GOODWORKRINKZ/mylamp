#include <unity.h>

#include <string>

#include "web/StatusJsonBuilder.h"

namespace {

void test_status_json_contains_build_and_runtime_fields() {
  lamp::web::StatusSnapshot snapshot;
  snapshot.version = "0.1.0-dev";
  snapshot.channel = "dev";
  snapshot.board = "esp32-c3-supermini";
  snapshot.networkMode = "ap";
  snapshot.networkStatus = "AP: MYLAMP";
  snapshot.clockStatus = "Clock: unavailable";
  snapshot.currentTime = "";
  snapshot.sensorStatus = "Sensor: unavailable";
  snapshot.sensorAvailable = false;
  snapshot.temperatureC = 0.0f;
  snapshot.humidityPercent = 0.0f;
  snapshot.activeEffect = "boot-solid";
  snapshot.activePresetId = "warm_waves";
  snapshot.activePresetName = "Warm Waves";
  snapshot.autoplayEnabled = true;
  snapshot.activePlaylistId = "evening";
  snapshot.liveErrorSummary = "";

  const std::string json = lamp::web::buildStatusJson(snapshot);

  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(json.find("\"version\":\"0.1.0-dev\"")));
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(json.find("\"channel\":\"dev\"")));
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(json.find("\"networkMode\":\"ap\"")));
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(json.find("\"sensorStatus\":\"Sensor: unavailable\"")));
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(json.find("\"temperatureC\":null")));
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(json.find("\"humidityPercent\":null")));
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(json.find("\"activeEffect\":\"boot-solid\"")));
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(json.find("\"activePresetId\":\"warm_waves\"")));
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(json.find("\"activePresetName\":\"Warm Waves\"")));
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(json.find("\"autoplayEnabled\":true")));
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(json.find("\"activePlaylistId\":\"evening\"")));
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(json.find("\"liveErrorSummary\":\"\"")));
}

void test_status_json_escapes_quotes_and_backslashes() {
  lamp::web::StatusSnapshot snapshot;
  snapshot.version = "0.1.0-dev";
  snapshot.channel = "dev";
  snapshot.board = "esp32-c3\\mini";
  snapshot.networkMode = "client";
  snapshot.networkStatus = "IP: \"192.168.1.55\"";
  snapshot.clockStatus = "Clock: NTP";
  snapshot.currentTime = "12:34:56";
  snapshot.sensorStatus = "Sensor: ok";
  snapshot.sensorAvailable = true;
  snapshot.temperatureC = 23.5f;
  snapshot.humidityPercent = 48.25f;
  snapshot.activeEffect = "debug-columns";
  snapshot.activePresetId = "soft_clock";
  snapshot.activePresetName = "Soft \"Clock\"";
  snapshot.autoplayEnabled = false;
  snapshot.activePlaylistId = "";
  snapshot.liveErrorSummary = "Ошибка в строке 4: hum()\\hint";

  const std::string json = lamp::web::buildStatusJson(snapshot);

  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(json.find("esp32-c3\\\\mini")));
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(json.find("\\\"192.168.1.55\\\"")));
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(json.find("\"currentTime\":\"12:34:56\"")));
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(json.find("\"temperatureC\":23.5")));
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(json.find("\"humidityPercent\":48.25")));
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(json.find("\"activePresetName\":\"Soft \\\"Clock\\\"\"")));
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(json.find("\"autoplayEnabled\":false")));
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(json.find("Ошибка в строке 4: hum()\\\\hint")));
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_status_json_contains_build_and_runtime_fields);
  RUN_TEST(test_status_json_escapes_quotes_and_backslashes);
  return UNITY_END();
}