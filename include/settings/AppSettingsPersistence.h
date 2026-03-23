#pragma once

#include <string>

#include "settings/AppSettings.h"

namespace lamp::settings {

class ISettingsBackend {
 public:
  virtual ~ISettingsBackend() = default;

  virtual bool getString(const char* key, std::string& value) const = 0;
  virtual bool getBool(const char* key, bool& value) const = 0;
  virtual void putString(const char* key, const std::string& value) = 0;
  virtual void putBool(const char* key, bool value) = 0;
};

class AppSettingsPersistence {
 public:
  AppSettings load(const ISettingsBackend& backend) const;
  void save(const AppSettings& settings, ISettingsBackend& backend) const;

 private:
  static const char* networkModeToString(network::NetworkMode mode);
  static network::NetworkMode networkModeFromString(const std::string& value);
  static std::string normalizeUpdateChannel(const std::string& value);
};

}  // namespace lamp::settings
