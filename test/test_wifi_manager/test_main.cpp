#include <unity.h>

#include <string>

#include "network/IWiFiAdapter.h"
#include "network/WiFiManager.h"
#include "settings/AppSettings.h"

namespace {

class FakeWiFiAdapter : public lamp::network::IWiFiAdapter {
 public:
  bool stationConnectResult = false;
  std::string localIpValue;
  std::string lastStartedAp;
  std::string lastStationSsid;
  std::string lastStationPassword;

  bool startAccessPoint(const std::string& ssid) override {
    lastStartedAp = ssid;
    return true;
  }

  bool connectStation(const std::string& ssid, const std::string& password) override {
    lastStationSsid = ssid;
    lastStationPassword = password;
    return stationConnectResult;
  }

  std::string localIp() const override {
    return localIpValue;
  }
};

void test_wifi_manager_uses_client_mode_when_station_connects() {
  lamp::settings::AppSettings settings;
  settings.network.preferredMode = lamp::network::NetworkMode::kClient;
  settings.network.clientSsid = "HomeWiFi";
  settings.network.clientPassword = "secret";

  FakeWiFiAdapter adapter;
  adapter.stationConnectResult = true;
  adapter.localIpValue = "192.168.1.77";

  lamp::network::WiFiManager manager;
  const lamp::network::WiFiStartupResult result = manager.startup(settings.network, adapter);

  TEST_ASSERT_EQUAL_INT(static_cast<int>(lamp::network::NetworkMode::kClient),
                        static_cast<int>(result.activeMode));
  TEST_ASSERT_EQUAL_STRING("HomeWiFi", adapter.lastStationSsid.c_str());
  TEST_ASSERT_EQUAL_STRING("192.168.1.77", result.ipAddress.c_str());
  TEST_ASSERT_EQUAL_STRING("192.168.1.77", result.statusLine.c_str());
}

void test_wifi_manager_falls_back_to_ap_when_station_fails() {
  lamp::settings::AppSettings settings;
  settings.network.preferredMode = lamp::network::NetworkMode::kClient;
  settings.network.clientSsid = "HomeWiFi";
  settings.network.accessPointName = "MYLAMP-AP";

  FakeWiFiAdapter adapter;
  adapter.stationConnectResult = false;

  lamp::network::WiFiManager manager;
  const lamp::network::WiFiStartupResult result = manager.startup(settings.network, adapter);

  TEST_ASSERT_EQUAL_INT(static_cast<int>(lamp::network::NetworkMode::kAccessPoint),
                        static_cast<int>(result.activeMode));
  TEST_ASSERT_EQUAL_STRING("MYLAMP-AP", adapter.lastStartedAp.c_str());
  TEST_ASSERT_EQUAL_STRING("AP: MYLAMP-AP", result.statusLine.c_str());
}

void test_wifi_manager_starts_access_point_directly_when_preferred() {
  lamp::settings::AppSettings settings;
  settings.network.preferredMode = lamp::network::NetworkMode::kAccessPoint;
  settings.network.accessPointName = "MYLAMP-LOCAL";

  FakeWiFiAdapter adapter;

  lamp::network::WiFiManager manager;
  const lamp::network::WiFiStartupResult result = manager.startup(settings.network, adapter);

  TEST_ASSERT_EQUAL_INT(static_cast<int>(lamp::network::NetworkMode::kAccessPoint),
                        static_cast<int>(result.activeMode));
  TEST_ASSERT_EQUAL_STRING("MYLAMP-LOCAL", adapter.lastStartedAp.c_str());
  TEST_ASSERT_EQUAL_STRING("AP: MYLAMP-LOCAL", result.statusLine.c_str());
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_wifi_manager_uses_client_mode_when_station_connects);
  RUN_TEST(test_wifi_manager_falls_back_to_ap_when_station_fails);
  RUN_TEST(test_wifi_manager_starts_access_point_directly_when_preferred);
  return UNITY_END();
}