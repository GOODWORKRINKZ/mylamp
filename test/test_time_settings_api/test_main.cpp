#include <unity.h>

#include <string>

#include "settings/AppSettings.h"
#include "web/TimeSettingsJson.h"

namespace {

void test_time_settings_json_contains_current_timezone() {
  lamp::settings::AppSettings settings;
  settings.clock.timezone = "MSK-3";

  const std::string json = lamp::web::buildTimeSettingsJson(settings);

  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(json.find("\"timezone\":\"MSK-3\"")));
}

void test_apply_time_settings_update_changes_timezone() {
  lamp::settings::AppSettings settings;

  const bool applied = lamp::web::applyTimeSettingsUpdate("MSK-3", settings);

  TEST_ASSERT_TRUE(applied);
  TEST_ASSERT_EQUAL_STRING("MSK-3", settings.clock.timezone.c_str());
}

void test_apply_time_settings_update_rejects_unknown_timezone() {
  lamp::settings::AppSettings settings;
  settings.clock.timezone = "UTC0";

  const bool applied = lamp::web::applyTimeSettingsUpdate("Mars/Base", settings);

  TEST_ASSERT_FALSE(applied);
  TEST_ASSERT_EQUAL_STRING("UTC0", settings.clock.timezone.c_str());
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_time_settings_json_contains_current_timezone);
  RUN_TEST(test_apply_time_settings_update_changes_timezone);
  RUN_TEST(test_apply_time_settings_update_rejects_unknown_timezone);
  return UNITY_END();
}