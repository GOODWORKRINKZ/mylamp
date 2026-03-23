#include "settings/AppSettingsPersistence.h"

namespace lamp::settings {

namespace {

constexpr char kNetworkModeKey[] = "network.mode";
constexpr char kNetworkApNameKey[] = "network.apName";
constexpr char kNetworkClientSsidKey[] = "network.clientSsid";
constexpr char kNetworkClientPasswordKey[] = "network.clientPassword";
constexpr char kClockEnabledKey[] = "clock.enabled";
constexpr char kClockCachedOfflineKey[] = "clock.cachedOffline";
constexpr char kUpdateChannelKey[] = "update.channel";

}  // namespace

AppSettings AppSettingsPersistence::load(const ISettingsBackend& backend) const {
  AppSettings settings;

  std::string stringValue;
  bool boolValue = false;

  if (backend.getString(kNetworkModeKey, stringValue)) {
    settings.network.preferredMode = networkModeFromString(stringValue);
  }
  if (backend.getString(kNetworkApNameKey, stringValue)) {
    settings.network.accessPointName = stringValue;
  }
  if (backend.getString(kNetworkClientSsidKey, stringValue)) {
    settings.network.clientSsid = stringValue;
  }
  if (backend.getString(kNetworkClientPasswordKey, stringValue)) {
    settings.network.clientPassword = stringValue;
  }
  if (backend.getBool(kClockEnabledKey, boolValue)) {
    settings.clock.enabled = boolValue;
  }
  if (backend.getBool(kClockCachedOfflineKey, boolValue)) {
    settings.clock.showCachedTimeWhenOffline = boolValue;
  }
  if (backend.getString(kUpdateChannelKey, stringValue)) {
    settings.update.channel = normalizeUpdateChannel(stringValue);
  }

  return settings;
}

void AppSettingsPersistence::save(const AppSettings& settings, ISettingsBackend& backend) const {
  backend.putString(kNetworkModeKey, networkModeToString(settings.network.preferredMode));
  backend.putString(kNetworkApNameKey, settings.network.accessPointName);
  backend.putString(kNetworkClientSsidKey, settings.network.clientSsid);
  backend.putString(kNetworkClientPasswordKey, settings.network.clientPassword);
  backend.putBool(kClockEnabledKey, settings.clock.enabled);
  backend.putBool(kClockCachedOfflineKey, settings.clock.showCachedTimeWhenOffline);
  backend.putString(kUpdateChannelKey, normalizeUpdateChannel(settings.update.channel));
}

const char* AppSettingsPersistence::networkModeToString(network::NetworkMode mode) {
  return mode == network::NetworkMode::kClient ? "client" : "ap";
}

network::NetworkMode AppSettingsPersistence::networkModeFromString(const std::string& value) {
  return value == "client" ? network::NetworkMode::kClient : network::NetworkMode::kAccessPoint;
}

std::string AppSettingsPersistence::normalizeUpdateChannel(const std::string& value) {
  return value == "dev" ? "dev" : "stable";
}

}  // namespace lamp::settings
