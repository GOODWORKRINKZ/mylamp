#include <unity.h>

#include "network/NetworkPlanner.h"
#include "settings/AppSettings.h"

namespace {

void test_client_mode_stays_client_when_wifi_is_available() {
  lamp::settings::AppSettings settings;
  settings.network.preferredMode = lamp::network::NetworkMode::kClient;
  settings.network.clientSsid = "HomeWiFi";

  lamp::network::NetworkPlanner planner;
  const lamp::network::PlannedNetworkState state =
      planner.planStartup(settings.network, true, true, "192.168.1.55");

  TEST_ASSERT_EQUAL_INT(static_cast<int>(lamp::network::NetworkMode::kClient),
                        static_cast<int>(state.activeMode));
  TEST_ASSERT_TRUE(state.timeSyncAllowed);
  TEST_ASSERT_TRUE(state.otaAllowed);
  TEST_ASSERT_EQUAL_STRING("192.168.1.55", state.statusLine.c_str());
}

void test_client_mode_falls_back_to_access_point_when_wifi_is_unavailable() {
  lamp::settings::AppSettings settings;
  settings.network.preferredMode = lamp::network::NetworkMode::kClient;
  settings.network.clientSsid = "HomeWiFi";

  lamp::network::NetworkPlanner planner;
  const lamp::network::PlannedNetworkState state =
      planner.planStartup(settings.network, false, false, "");

  TEST_ASSERT_EQUAL_INT(static_cast<int>(lamp::network::NetworkMode::kAccessPoint),
                        static_cast<int>(state.activeMode));
  TEST_ASSERT_FALSE(state.timeSyncAllowed);
  TEST_ASSERT_FALSE(state.otaAllowed);
  TEST_ASSERT_EQUAL_STRING("AP: MYLAMP", state.statusLine.c_str());
}

void test_access_point_mode_prefers_local_live_coding() {
  lamp::settings::AppSettings settings;
  settings.network.preferredMode = lamp::network::NetworkMode::kAccessPoint;
  settings.network.accessPointName = "MYLAMP-ABCD";

  lamp::network::NetworkPlanner planner;
  const lamp::network::PlannedNetworkState state =
      planner.planStartup(settings.network, false, false, "");

  TEST_ASSERT_EQUAL_INT(static_cast<int>(lamp::network::NetworkMode::kAccessPoint),
                        static_cast<int>(state.activeMode));
  TEST_ASSERT_TRUE(state.liveCodingAllowed);
  TEST_ASSERT_EQUAL_STRING("AP: MYLAMP-ABCD", state.statusLine.c_str());
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_client_mode_stays_client_when_wifi_is_available);
  RUN_TEST(test_client_mode_falls_back_to_access_point_when_wifi_is_unavailable);
  RUN_TEST(test_access_point_mode_prefers_local_live_coding);
  return UNITY_END();
}