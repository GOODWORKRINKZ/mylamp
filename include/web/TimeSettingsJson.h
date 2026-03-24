#pragma once

#include <string>

#include "settings/AppSettings.h"

namespace lamp::web {

std::string buildTimeSettingsJson(const settings::AppSettings& settings);
bool applyTimeSettingsUpdate(const std::string& timezone, settings::AppSettings& settings);

}  // namespace lamp::web