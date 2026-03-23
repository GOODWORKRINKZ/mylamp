#include "network/NetworkPlanner.h"

namespace lamp::network {

PlannedNetworkState NetworkPlanner::planStartup(const NetworkSettings& settings, bool wifiAvailable,
                                                bool internetAvailable,
                                                const std::string& ipAddress) const {
  PlannedNetworkState state;
  state.liveCodingAllowed = true;

  const bool hasClientConfig = !settings.clientSsid.empty();
  if (settings.preferredMode == NetworkMode::kClient && hasClientConfig && wifiAvailable) {
    state.activeMode = NetworkMode::kClient;
    state.timeSyncAllowed = internetAvailable;
    state.otaAllowed = internetAvailable;
    state.statusLine = ipAddress.empty() ? std::string("IP: pending") : ipAddress;
    return state;
  }

  state.activeMode = NetworkMode::kAccessPoint;
  state.timeSyncAllowed = false;
  state.otaAllowed = false;
  state.statusLine = "AP: " + settings.accessPointName;
  return state;
}

}  // namespace lamp::network
