#include "network/WiFiManager.h"

namespace lamp::network {

WiFiStartupResult WiFiManager::startup(const NetworkSettings& settings, IWiFiAdapter& adapter) const {
  WiFiStartupResult result;

  const bool wantsClient = settings.preferredMode == NetworkMode::kClient;
  const bool hasClientConfig = !settings.clientSsid.empty();
  if (wantsClient && hasClientConfig &&
      adapter.connectStation(settings.clientSsid, settings.clientPassword)) {
    result.activeMode = NetworkMode::kClient;
    result.ipAddress = adapter.localIp();
    result.statusLine = result.ipAddress.empty() ? std::string("IP: pending") : result.ipAddress;
    return result;
  }

  adapter.startAccessPoint(settings.accessPointName);
  result.activeMode = NetworkMode::kAccessPoint;
  result.statusLine = "AP: " + settings.accessPointName;
  return result;
}

}  // namespace lamp::network
