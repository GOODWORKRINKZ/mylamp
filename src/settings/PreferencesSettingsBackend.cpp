#include "settings/PreferencesSettingsBackend.h"

namespace lamp::settings {

PreferencesSettingsBackend::PreferencesSettingsBackend(const char* namespaceName) {
  namespaceName_ = namespaceName == nullptr ? "mylamp" : namespaceName;
}

PreferencesSettingsBackend::~PreferencesSettingsBackend() {
  if (ready_) {
    preferences_.end();
  }
}

bool PreferencesSettingsBackend::begin() {
  if (ready_) {
    return true;
  }

  ready_ = preferences_.begin(namespaceName_.c_str(), false);
  return ready_;
}

bool PreferencesSettingsBackend::isReady() const {
  return ready_;
}

bool PreferencesSettingsBackend::getString(const char* key, std::string& value) const {
  if (!ready_) {
    return false;
  }

  const String stored = preferences_.getString(key, "");
  if (stored.isEmpty()) {
    return false;
  }
  value = stored.c_str();
  return true;
}

bool PreferencesSettingsBackend::getBool(const char* key, bool& value) const {
  if (!ready_) {
    return false;
  }

  if (!preferences_.isKey(key)) {
    return false;
  }
  value = preferences_.getBool(key, false);
  return true;
}

void PreferencesSettingsBackend::putString(const char* key, const std::string& value) {
  if (!ready_) {
    return;
  }

  preferences_.putString(key, value.c_str());
}

void PreferencesSettingsBackend::putBool(const char* key, bool value) {
  if (!ready_) {
    return;
  }

  preferences_.putBool(key, value);
}

}  // namespace lamp::settings
