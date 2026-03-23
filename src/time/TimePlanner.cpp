#include "time/TimePlanner.h"

namespace lamp::time {

PlannedTimeState TimePlanner::plan(const ClockSettings& settings,
                                   const network::PlannedNetworkState& networkState,
                                   bool hasCachedTime) const {
  PlannedTimeState state;

  if (!settings.enabled) {
    state.statusLine = "Clock: disabled";
    return state;
  }

  if (networkState.timeSyncAllowed) {
    state.ntpSyncEnabled = true;
    state.clockOverlayVisible = true;
    state.statusLine = "Clock: NTP";
    return state;
  }

  if (settings.showCachedTimeWhenOffline && hasCachedTime) {
    state.clockOverlayVisible = true;
    state.usingCachedTime = true;
    state.statusLine = "Clock: cached";
    return state;
  }

  state.statusLine = "Clock: unavailable";
  return state;
}

}  // namespace lamp::time
