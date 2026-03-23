#include "settings/PreferencesSettingsBackend.h"

namespace lamp::settings {

PreferencesSettingsBackend::PreferencesSettingsBackend(const char* namespaceName) {
  preferences_.begin(namespaceName, false);
}

PreferencesSettingsBackend::~PreferencesSettingsBackend() {
  preferences_.end();
}

bool PreferencesSettingsBackend::getString(const char* key, std::string& value) const {
  const String stored = preferences_.getString(key, "");
  if (stored.isEmpty()) {
    return false;
  }
  value = stored.c_str();
  return true;
}

bool PreferencesSettingsBackend::getBool(const char* key, bool& value) const {
  if (!preferences_.isKey(key)) {
    return false;
  }
  value = preferences_.getBool(key, false);
  return true;
}

void PreferencesSettingsBackend::putString(const char* key, const std::string& value) {
  preferences_.putString(key, value.c_str());
}

void PreferencesSettingsBackend::putBool(const char* key, bool value) {
  preferences_.putBool(key, value);
}

}  // namespace lamp::settings
