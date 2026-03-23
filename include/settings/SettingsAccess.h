#pragma once

#include "settings/AppSettingsPersistence.h"

namespace lamp::settings {

inline bool loadSettingsIfReady(const ISettingsBackend& backend, AppSettings& settings) {
  if (!backend.isReady()) {
    return false;
  }

  AppSettingsPersistence persistence;
  settings = persistence.load(backend);
  return true;
}

inline bool saveSettingsIfReady(const AppSettings& settings, ISettingsBackend& backend) {
  if (!backend.isReady()) {
    return false;
  }

  AppSettingsPersistence persistence;
  persistence.save(settings, backend);
  return true;
}

}  // namespace lamp::settings