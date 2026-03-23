#pragma once

#include <string>

#include "network/NetworkPlanner.h"

namespace lamp::time {

struct ClockSettings {
  bool enabled = true;
  bool showCachedTimeWhenOffline = true;
  std::string timezone = "UTC0";
};

struct PlannedTimeState {
  bool ntpSyncEnabled = false;
  bool clockOverlayVisible = false;
  bool usingCachedTime = false;
  std::string statusLine;
};

class TimePlanner {
 public:
  PlannedTimeState plan(const ClockSettings& settings,
                        const network::PlannedNetworkState& networkState,
                        bool hasCachedTime) const;
};

}  // namespace lamp::time
