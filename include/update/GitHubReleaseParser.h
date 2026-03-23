#pragma once

#include <string>

#include "update/FirmwareReleaseInfo.h"

namespace lamp::update {

class GitHubReleaseParser {
 public:
  FirmwareReleaseInfo parse(const std::string& payload, const std::string& currentVersion,
                            const std::string& channel,
                            const std::string& hardwareType) const;
};

}  // namespace lamp::update