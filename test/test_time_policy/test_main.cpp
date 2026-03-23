#include <unity.h>

#include "network/NetworkPlanner.h"
#include "settings/AppSettings.h"
#include "time/TimePlanner.h"

namespace {

void test_client_mode_with_internet_enables_ntp_sync() {
  lamp::settings::AppSettings settings;
  lamp::network::PlannedNetworkState networkState;
  networkState.activeMode = lamp::network::NetworkMode::kClient;
  networkState.timeSyncAllowed = true;

  lamp::time::TimePlanner planner;
  const lamp::time::PlannedTimeState timeState =
      planner.plan(settings.clock, networkState, false);

  TEST_ASSERT_TRUE(timeState.ntpSyncEnabled);
  TEST_ASSERT_TRUE(timeState.clockOverlayVisible);
  TEST_ASSERT_FALSE(timeState.usingCachedTime);
  TEST_ASSERT_EQUAL_STRING("Clock: NTP", timeState.statusLine.c_str());
}

void test_access_point_mode_can_show_cached_time() {
  lamp::settings::AppSettings settings;
  settings.clock.showCachedTimeWhenOffline = true;
  lamp::network::PlannedNetworkState networkState;
  networkState.activeMode = lamp::network::NetworkMode::kAccessPoint;
  networkState.timeSyncAllowed = false;

  lamp::time::TimePlanner planner;
  const lamp::time::PlannedTimeState timeState =
      planner.plan(settings.clock, networkState, true);

  TEST_ASSERT_FALSE(timeState.ntpSyncEnabled);
  TEST_ASSERT_TRUE(timeState.clockOverlayVisible);
  TEST_ASSERT_TRUE(timeState.usingCachedTime);
  TEST_ASSERT_EQUAL_STRING("Clock: cached", timeState.statusLine.c_str());
}

void test_access_point_mode_hides_clock_without_cached_time() {
  lamp::settings::AppSettings settings;
  settings.clock.showCachedTimeWhenOffline = true;
  lamp::network::PlannedNetworkState networkState;
  networkState.activeMode = lamp::network::NetworkMode::kAccessPoint;
  networkState.timeSyncAllowed = false;

  lamp::time::TimePlanner planner;
  const lamp::time::PlannedTimeState timeState =
      planner.plan(settings.clock, networkState, false);

  TEST_ASSERT_FALSE(timeState.ntpSyncEnabled);
  TEST_ASSERT_FALSE(timeState.clockOverlayVisible);
  TEST_ASSERT_FALSE(timeState.usingCachedTime);
  TEST_ASSERT_EQUAL_STRING("Clock: unavailable", timeState.statusLine.c_str());
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_client_mode_with_internet_enables_ntp_sync);
  RUN_TEST(test_access_point_mode_can_show_cached_time);
  RUN_TEST(test_access_point_mode_hides_clock_without_cached_time);
  return UNITY_END();
}