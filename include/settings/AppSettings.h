#pragma once

#include <string>

#include "AppConfig.h"
#include "network/NetworkPlanner.h"

namespace lamp::settings {

struct AppSettings {
  network::NetworkSettings network;

  AppSettings() {
    network.accessPointName = config::kAccessPointPrefix;
  }
};

}  // namespace lamp::settings
