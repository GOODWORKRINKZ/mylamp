#pragma once

#include <string>

#include "AppConfig.h"
#include "network/NetworkPlanner.h"
#include "time/TimePlanner.h"

namespace lamp::settings {

struct AppSettings {
  network::NetworkSettings network;
  time::ClockSettings clock;

  AppSettings() {
    network.accessPointName = config::kAccessPointPrefix;
  }
};

}  // namespace lamp::settings
