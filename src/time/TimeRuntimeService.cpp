#include "time/TimeRuntimeService.h"

#include "AppConfig.h"

namespace lamp::time {

RuntimeTimeState TimeRuntimeService::refresh(const PlannedTimeState& plan,
                                             ITimeSource& timeSource) const {
  RuntimeTimeState state;
  state.statusLine = plan.statusLine;

  if (plan.ntpSyncEnabled) {
    const bool synced = timeSource.syncTime(config::kTimeZone, config::kNtpPrimaryServer,
                                            config::kNtpSecondaryServer);
    state.hasValidTime = synced && timeSource.hasValidTime();
    state.currentTime = state.hasValidTime ? timeSource.formattedTime() : "";
    return state;
  }

  if (plan.usingCachedTime && timeSource.hasValidTime()) {
    state.hasValidTime = true;
    state.currentTime = timeSource.formattedTime();
    return state;
  }

  return state;
}

}  // namespace lamp::time
