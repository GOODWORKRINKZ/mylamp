#pragma once

#include <string>

#include "time/ITimeSource.h"
#include "time/TimePlanner.h"

namespace lamp::time {

struct RuntimeTimeState {
  bool hasValidTime = false;
  std::string currentTime;
  std::string statusLine;
};

class TimeRuntimeService {
 public:
  RuntimeTimeState refresh(const PlannedTimeState& plan, ITimeSource& timeSource) const;
};

}  // namespace lamp::time
