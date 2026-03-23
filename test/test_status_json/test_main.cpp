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
  snapshot.activeEffect = "boot-solid";

  const std::string json = lamp::web::buildStatusJson(snapshot);

  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(json.find("\"version\":\"0.1.0-dev\"")));
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(json.find("\"channel\":\"dev\"")));
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(json.find("\"networkMode\":\"ap\"")));
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(json.find("\"activeEffect\":\"boot-solid\"")));
}

void test_status_json_escapes_quotes_and_backslashes() {
  lamp::web::StatusSnapshot snapshot;
  snapshot.version = "0.1.0-dev";
  snapshot.channel = "dev";
  snapshot.board = "esp32-c3\\mini";
  snapshot.networkMode = "client";
  snapshot.networkStatus = "IP: \"192.168.1.55\"";
  snapshot.clockStatus = "Clock: NTP";
  snapshot.activeEffect = "debug-columns";

  const std::string json = lamp::web::buildStatusJson(snapshot);

  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(json.find("esp32-c3\\\\mini")));
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(json.find("\\\"192.168.1.55\\\"")));
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