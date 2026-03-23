#pragma once

#include <string>

#include "network/IWiFiAdapter.h"
#include "network/NetworkPlanner.h"

namespace lamp::network {

struct WiFiStartupResult {
  NetworkMode activeMode = NetworkMode::kAccessPoint;
  std::string ipAddress;
  std::string statusLine;
};

class WiFiManager {
 public:
  WiFiStartupResult startup(const NetworkSettings& settings, IWiFiAdapter& adapter) const;
};

}  // namespace lamp::network
