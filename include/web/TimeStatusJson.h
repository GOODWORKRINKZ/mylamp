#pragma once

#include <string>

namespace lamp::web {

struct TimeStatusSnapshot {
  std::string currentTime;
  std::string timezone;
  std::string syncStatus;   // ntp_synced | ntp_pending | ntp_failed | ntp_disabled | cached
  std::string ntpServer;
  long epoch = 0;
};

std::string buildTimeStatusJson(const TimeStatusSnapshot& snapshot);

}  // namespace lamp::web
