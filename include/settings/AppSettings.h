#pragma once

#include <string>

#include "AppConfig.h"
#include "network/NetworkPlanner.h"
#include "time/TimePlanner.h"

namespace lamp::settings {

struct UpdateSettings {
  std::string channel = "stable";
};

struct AppSettings {
  network::NetworkSettings network;
  time::ClockSettings clock;
  UpdateSettings update;

  AppSettings() {
    network.accessPointName = config::kAccessPointPrefix;
    clock.timezone = config::kTimeZone;
  }
};

}  // namespace lamp::settings
