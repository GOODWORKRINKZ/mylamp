#pragma once

#include <string>

namespace lamp::network {

enum class NetworkMode {
  kAccessPoint,
  kClient,
};

struct NetworkSettings {
  NetworkMode preferredMode = NetworkMode::kAccessPoint;
  std::string accessPointName;
  std::string clientSsid;
  std::string clientPassword;
};

struct PlannedNetworkState {
  NetworkMode activeMode = NetworkMode::kAccessPoint;
  bool liveCodingAllowed = true;
  bool timeSyncAllowed = false;
  bool otaAllowed = false;
  std::string statusLine;
};

class NetworkPlanner {
 public:
  PlannedNetworkState planStartup(const NetworkSettings& settings, bool wifiAvailable,
                                  bool internetAvailable, const std::string& ipAddress) const;
};

}  // namespace lamp::network
