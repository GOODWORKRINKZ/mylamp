#include "web/NetworkSettingsJson.h"

#include "AppConfig.h"

namespace lamp::web {

namespace {

std::string escapeJson(const std::string& input) {
  std::string output;
  output.reserve(input.size());

  for (char ch : input) {
    switch (ch) {
      case '\\':
        output += "\\\\";
        break;
      case '"':
        output += "\\\"";
        break;
      default:
        output += ch;
        break;
    }
  }

  return output;
}

const char* networkModeToString(lamp::network::NetworkMode mode) {
  return mode == lamp::network::NetworkMode::kClient ? "client" : "ap";
}

}  // namespace

std::string buildNetworkSettingsJson(const settings::AppSettings& settings) {
  std::string json = "{";
  json += "\"mode\":\"" + std::string(networkModeToString(settings.network.preferredMode)) + "\",";
  json += "\"accessPointName\":\"" + escapeJson(settings.network.accessPointName) + "\",";
  json += "\"clientSsid\":\"" + escapeJson(settings.network.clientSsid) + "\"";
  json += "}";
  return json;
}

bool applyNetworkSettingsUpdate(const std::string& mode, const std::string& accessPointName,
                                const std::string& clientSsid,
                                const std::string& clientPassword,
                                settings::AppSettings& settings) {
  if (mode == "ap") {
    settings.network.preferredMode = lamp::network::NetworkMode::kAccessPoint;
  } else if (mode == "client") {
    settings.network.preferredMode = lamp::network::NetworkMode::kClient;
  } else {
    return false;
  }

  settings.network.accessPointName = accessPointName.empty() ? lamp::config::kAccessPointPrefix
                                                             : accessPointName;
  settings.network.clientSsid = clientSsid;
  settings.network.clientPassword = clientPassword;
  return true;
}

}  // namespace lamp::web
