#pragma once

#include <string>

#include "update/FirmwareReleaseInfo.h"

namespace lamp::update {

class GitHubReleaseParser {
 public:
   FirmwareReleaseInfo parse(const char* payload, const std::string& currentVersion,
                                          const std::string& channel,
                                          const std::string& hardwareType) const;
};

}  // namespace lamp::update