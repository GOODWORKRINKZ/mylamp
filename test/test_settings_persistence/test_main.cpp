#include <unity.h>

#include <map>
#include <string>

#include "settings/AppSettingsPersistence.h"

namespace {

class FakeSettingsBackend : public lamp::settings::ISettingsBackend {
 public:
  std::map<std::string, std::string> values;

  bool getString(const char* key, std::string& value) const override {
    const auto it = values.find(key);
    if (it == values.end()) {
      return false;
    }
    value = it->second;
    return true;
  }

  bool getBool(const char* key, bool& value) const override {
    const auto it = values.find(key);
    if (it == values.end()) {
      return false;
    }
    value = it->second == "true";
    return true;
  }

  void putString(const char* key, const std::string& value) override {
    values[key] = value;
  }

  void putBool(const char* key, bool value) override {
    values[key] = value ? "true" : "false";
  }
};

void test_load_uses_defaults_when_storage_is_empty() {
  FakeSettingsBackend backend;
  lamp::settings::AppSettingsPersistence persistence;

  const lamp::settings::AppSettings settings = persistence.load(backend);

  TEST_ASSERT_EQUAL_INT(static_cast<int>(lamp::network::NetworkMode::kAccessPoint),
                        static_cast<int>(settings.network.preferredMode));
  TEST_ASSERT_EQUAL_STRING("MYLAMP", settings.network.accessPointName.c_str());
  TEST_ASSERT_TRUE(settings.clock.enabled);
}

void test_load_reads_saved_network_and_clock_settings() {
  FakeSettingsBackend backend;
  backend.values["network.mode"] = "client";
  backend.values["network.apName"] = "MYLAMP-ROOM";
  backend.values["network.clientSsid"] = "HomeWiFi";
  backend.values["network.clientPassword"] = "secret";
  backend.values["clock.enabled"] = "false";
  backend.values["clock.cachedOffline"] = "false";

  lamp::settings::AppSettingsPersistence persistence;
  const lamp::settings::AppSettings settings = persistence.load(backend);

  TEST_ASSERT_EQUAL_INT(static_cast<int>(lamp::network::NetworkMode::kClient),
                        static_cast<int>(settings.network.preferredMode));
  TEST_ASSERT_EQUAL_STRING("MYLAMP-ROOM", settings.network.accessPointName.c_str());
  TEST_ASSERT_EQUAL_STRING("HomeWiFi", settings.network.clientSsid.c_str());
  TEST_ASSERT_FALSE(settings.clock.enabled);
  TEST_ASSERT_FALSE(settings.clock.showCachedTimeWhenOffline);
}

void test_save_persists_network_and_clock_settings() {
  FakeSettingsBackend backend;
  lamp::settings::AppSettings settings;
  settings.network.preferredMode = lamp::network::NetworkMode::kClient;
  settings.network.accessPointName = "MYLAMP-ROOM";
  settings.network.clientSsid = "HomeWiFi";
  settings.network.clientPassword = "secret";
  settings.clock.enabled = true;
  settings.clock.showCachedTimeWhenOffline = false;

  lamp::settings::AppSettingsPersistence persistence;
  persistence.save(settings, backend);

  TEST_ASSERT_EQUAL_STRING("client", backend.values["network.mode"].c_str());
  TEST_ASSERT_EQUAL_STRING("MYLAMP-ROOM", backend.values["network.apName"].c_str());
  TEST_ASSERT_EQUAL_STRING("HomeWiFi", backend.values["network.clientSsid"].c_str());
  TEST_ASSERT_EQUAL_STRING("secret", backend.values["network.clientPassword"].c_str());
  TEST_ASSERT_EQUAL_STRING("true", backend.values["clock.enabled"].c_str());
  TEST_ASSERT_EQUAL_STRING("false", backend.values["clock.cachedOffline"].c_str());
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_load_uses_defaults_when_storage_is_empty);
  RUN_TEST(test_load_reads_saved_network_and_clock_settings);
  RUN_TEST(test_save_persists_network_and_clock_settings);
  return UNITY_END();
}