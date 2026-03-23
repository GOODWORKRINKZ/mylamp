#include <unity.h>

#include <string>

#include "settings/AppSettings.h"
#include "web/NetworkSettingsJson.h"

namespace {

void test_network_settings_json_contains_current_values() {
  lamp::settings::AppSettings settings;
  settings.network.preferredMode = lamp::network::NetworkMode::kClient;
  settings.network.accessPointName = "MYLAMP-ROOM";
  settings.network.clientSsid = "HomeWiFi";
  settings.network.clientPassword = "secret";

  const std::string json = lamp::web::buildNetworkSettingsJson(settings);

  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(json.find("\"mode\":\"client\"")));
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(json.find("\"accessPointName\":\"MYLAMP-ROOM\"")));
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(json.find("\"clientSsid\":\"HomeWiFi\"")));
}

void test_apply_network_settings_update_changes_target_settings() {
  lamp::settings::AppSettings settings;

  const bool applied = lamp::web::applyNetworkSettingsUpdate(
      "client", "MYLAMP-ROOM", "HomeWiFi", "secret", settings);

  TEST_ASSERT_TRUE(applied);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(lamp::network::NetworkMode::kClient),
                        static_cast<int>(settings.network.preferredMode));
  TEST_ASSERT_EQUAL_STRING("MYLAMP-ROOM", settings.network.accessPointName.c_str());
  TEST_ASSERT_EQUAL_STRING("HomeWiFi", settings.network.clientSsid.c_str());
  TEST_ASSERT_EQUAL_STRING("secret", settings.network.clientPassword.c_str());
}

void test_apply_network_settings_update_rejects_unknown_mode() {
  lamp::settings::AppSettings settings;

  const bool applied = lamp::web::applyNetworkSettingsUpdate(
      "invalid", "MYLAMP-ROOM", "HomeWiFi", "secret", settings);

  TEST_ASSERT_FALSE(applied);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(lamp::network::NetworkMode::kAccessPoint),
                        static_cast<int>(settings.network.preferredMode));
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_network_settings_json_contains_current_values);
  RUN_TEST(test_apply_network_settings_update_changes_target_settings);
  RUN_TEST(test_apply_network_settings_update_rejects_unknown_mode);
  return UNITY_END();
}