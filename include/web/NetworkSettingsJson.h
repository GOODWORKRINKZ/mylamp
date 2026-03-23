#pragma once

#include <string>

#include "settings/AppSettings.h"

namespace lamp::web {

std::string buildNetworkSettingsJson(const settings::AppSettings& settings);
bool applyNetworkSettingsUpdate(const std::string& mode, const std::string& accessPointName,
                                const std::string& clientSsid,
                                const std::string& clientPassword,
                                settings::AppSettings& settings);

}  // namespace lamp::web
