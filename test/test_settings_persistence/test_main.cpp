#include <unity.h>

#include <map>
#include <string>
#include <type_traits>
#include <utility>

#include "settings/AppSettingsPersistence.h"

namespace {

class FakeSettingsBackend final : public lamp::settings::ISettingsBackend {
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

template <typename T, typename = void>
struct has_update_channel : std::false_type {};

template <typename T>
struct has_update_channel<T, std::void_t<decltype(std::declval<T>().update.channel)>>
    : std::true_type {};

template <typename T, typename = void>
struct has_clock_timezone : std::false_type {};

template <typename T>
struct has_clock_timezone<T, std::void_t<decltype(std::declval<T>().clock.timezone)>>
  : std::true_type {};

void test_app_settings_exposes_update_channel() {
  TEST_ASSERT_TRUE(has_update_channel<lamp::settings::AppSettings>::value);
}

void test_app_settings_exposes_clock_timezone() {
  TEST_ASSERT_TRUE(has_clock_timezone<lamp::settings::AppSettings>::value);
}

void test_load_uses_defaults_when_storage_is_empty() {
  FakeSettingsBackend backend;
  lamp::settings::AppSettingsPersistence persistence;

  const lamp::settings::AppSettings settings = persistence.load(backend);

  TEST_ASSERT_EQUAL_INT(static_cast<int>(lamp::network::NetworkMode::kAccessPoint),
                        static_cast<int>(settings.network.preferredMode));
  TEST_ASSERT_EQUAL_STRING("MYLAMP", settings.network.accessPointName.c_str());
  TEST_ASSERT_TRUE(settings.clock.enabled);
  TEST_ASSERT_EQUAL_STRING("UTC0", settings.clock.timezone.c_str());
  TEST_ASSERT_EQUAL_STRING("stable", settings.update.channel.c_str());
}

void test_load_reads_saved_network_clock_and_update_settings() {
  FakeSettingsBackend backend;
  backend.values["network.mode"] = "client";
  backend.values["network.apName"] = "MYLAMP-ROOM";
  backend.values["network.clientSsid"] = "HomeWiFi";
  backend.values["network.clientPassword"] = "secret";
  backend.values["clock.enabled"] = "false";
  backend.values["clock.cachedOffline"] = "false";
  backend.values["clock.timezone"] = "MSK-3";
  backend.values["update.channel"] = "dev";

  lamp::settings::AppSettingsPersistence persistence;
  const lamp::settings::AppSettings settings = persistence.load(backend);

  TEST_ASSERT_EQUAL_INT(static_cast<int>(lamp::network::NetworkMode::kClient),
                        static_cast<int>(settings.network.preferredMode));
  TEST_ASSERT_EQUAL_STRING("MYLAMP-ROOM", settings.network.accessPointName.c_str());
  TEST_ASSERT_EQUAL_STRING("HomeWiFi", settings.network.clientSsid.c_str());
  TEST_ASSERT_FALSE(settings.clock.enabled);
  TEST_ASSERT_FALSE(settings.clock.showCachedTimeWhenOffline);
  TEST_ASSERT_EQUAL_STRING("MSK-3", settings.clock.timezone.c_str());
  TEST_ASSERT_EQUAL_STRING("dev", settings.update.channel.c_str());
}

void test_load_normalizes_invalid_update_channel_to_stable() {
  FakeSettingsBackend backend;
  backend.values["update.channel"] = "beta";

  lamp::settings::AppSettingsPersistence persistence;
  const lamp::settings::AppSettings settings = persistence.load(backend);

  TEST_ASSERT_EQUAL_STRING("stable", settings.update.channel.c_str());
}

void test_save_persists_network_clock_and_update_settings() {
  FakeSettingsBackend backend;
  lamp::settings::AppSettings settings;
  settings.network.preferredMode = lamp::network::NetworkMode::kClient;
  settings.network.accessPointName = "MYLAMP-ROOM";
  settings.network.clientSsid = "HomeWiFi";
  settings.network.clientPassword = "secret";
  settings.clock.enabled = true;
  settings.clock.showCachedTimeWhenOffline = false;
  settings.clock.timezone = "MSK-3";
  settings.update.channel = "dev";

  lamp::settings::AppSettingsPersistence persistence;
  persistence.save(settings, backend);

  TEST_ASSERT_EQUAL_STRING("client", backend.values["network.mode"].c_str());
  TEST_ASSERT_EQUAL_STRING("MYLAMP-ROOM", backend.values["network.apName"].c_str());
  TEST_ASSERT_EQUAL_STRING("HomeWiFi", backend.values["network.clientSsid"].c_str());
  TEST_ASSERT_EQUAL_STRING("secret", backend.values["network.clientPassword"].c_str());
  TEST_ASSERT_EQUAL_STRING("true", backend.values["clock.enabled"].c_str());
  TEST_ASSERT_EQUAL_STRING("false", backend.values["clock.cachedOffline"].c_str());
  TEST_ASSERT_EQUAL_STRING("MSK-3", backend.values["clock.timezone"].c_str());
  TEST_ASSERT_EQUAL_STRING("dev", backend.values["update.channel"].c_str());
}

void test_save_normalizes_invalid_update_channel_to_stable() {
  FakeSettingsBackend backend;
  lamp::settings::AppSettings settings;
  settings.update.channel = "beta";

  lamp::settings::AppSettingsPersistence persistence;
  persistence.save(settings, backend);

  TEST_ASSERT_EQUAL_STRING("stable", backend.values["update.channel"].c_str());
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_app_settings_exposes_update_channel);
  RUN_TEST(test_app_settings_exposes_clock_timezone);
  RUN_TEST(test_load_uses_defaults_when_storage_is_empty);
  RUN_TEST(test_load_reads_saved_network_clock_and_update_settings);
  RUN_TEST(test_load_normalizes_invalid_update_channel_to_stable);
  RUN_TEST(test_save_persists_network_clock_and_update_settings);
  RUN_TEST(test_save_normalizes_invalid_update_channel_to_stable);
  return UNITY_END();
}