#include <unity.h>

#include <string>

#include "time/ITimeSource.h"
#include "time/TimeRuntimeService.h"

namespace {

class FakeTimeSource : public lamp::time::ITimeSource {
 public:
  bool syncCalled = false;
  bool syncResult = false;
  bool cachedValid = false;
  std::string formattedValue;

  bool syncTime(const char* timezone, const char* primaryServer,
                const char* secondaryServer) override {
    (void)timezone;
    (void)primaryServer;
    (void)secondaryServer;
    syncCalled = true;
    cachedValid = syncResult;
    return syncResult;
  }

  bool hasValidTime() const override {
    return cachedValid;
  }

  std::string formattedTime() const override {
    return formattedValue;
  }
};

void test_runtime_service_syncs_time_when_ntp_is_enabled() {
  FakeTimeSource source;
  source.syncResult = true;
  source.formattedValue = "12:34:56";
  lamp::time::PlannedTimeState plan;
  plan.ntpSyncEnabled = true;
  plan.clockOverlayVisible = true;
  plan.statusLine = "Clock: NTP";

  lamp::time::TimeRuntimeService service;
  const lamp::time::RuntimeTimeState state = service.refresh(plan, source);

  TEST_ASSERT_TRUE(source.syncCalled);
  TEST_ASSERT_TRUE(state.hasValidTime);
  TEST_ASSERT_EQUAL_STRING("12:34:56", state.currentTime.c_str());
  TEST_ASSERT_EQUAL_STRING("Clock: NTP", state.statusLine.c_str());
}

void test_runtime_service_uses_cached_time_offline() {
  FakeTimeSource source;
  source.cachedValid = true;
  source.formattedValue = "23:59:01";
  lamp::time::PlannedTimeState plan;
  plan.usingCachedTime = true;
  plan.clockOverlayVisible = true;
  plan.statusLine = "Clock: cached";

  lamp::time::TimeRuntimeService service;
  const lamp::time::RuntimeTimeState state = service.refresh(plan, source);

  TEST_ASSERT_FALSE(source.syncCalled);
  TEST_ASSERT_TRUE(state.hasValidTime);
  TEST_ASSERT_EQUAL_STRING("23:59:01", state.currentTime.c_str());
  TEST_ASSERT_EQUAL_STRING("Clock: cached", state.statusLine.c_str());
}

void test_runtime_service_returns_empty_time_when_unavailable() {
  FakeTimeSource source;
  lamp::time::PlannedTimeState plan;
  plan.statusLine = "Clock: unavailable";

  lamp::time::TimeRuntimeService service;
  const lamp::time::RuntimeTimeState state = service.refresh(plan, source);

  TEST_ASSERT_FALSE(source.syncCalled);
  TEST_ASSERT_FALSE(state.hasValidTime);
  TEST_ASSERT_EQUAL_STRING("", state.currentTime.c_str());
  TEST_ASSERT_EQUAL_STRING("Clock: unavailable", state.statusLine.c_str());
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_runtime_service_syncs_time_when_ntp_is_enabled);
  RUN_TEST(test_runtime_service_uses_cached_time_offline);
  RUN_TEST(test_runtime_service_returns_empty_time_when_unavailable);
  return UNITY_END();
}