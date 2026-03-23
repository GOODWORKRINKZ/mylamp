#pragma once

#include <Preferences.h>

#include <string>

#include "settings/AppSettingsPersistence.h"

namespace lamp::settings {

class PreferencesSettingsBackend : public ISettingsBackend {
 public:
  explicit PreferencesSettingsBackend(const char* namespaceName = "mylamp");
  ~PreferencesSettingsBackend() override;

  bool getString(const char* key, std::string& value) const override;
  bool getBool(const char* key, bool& value) const override;
  void putString(const char* key, const std::string& value) override;
  void putBool(const char* key, bool value) override;

 private:
  mutable Preferences preferences_;
};

}  // namespace lamp::settings
