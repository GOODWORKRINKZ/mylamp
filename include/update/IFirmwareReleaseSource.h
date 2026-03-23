#pragma once

#include <string>

#include "settings/AppSettings.h"
#include "update/FirmwareReleaseInfo.h"

namespace lamp::update {

struct BuildIdentity {
  std::string projectName;
  std::string version;
  std::string channel;
  std::string board;
  std::string hardwareType;
};

class IFirmwareReleaseSource {
 public:
  virtual ~IFirmwareReleaseSource() = default;

  virtual FirmwareReleaseInfo check(const BuildIdentity& buildIdentity,
                                    const settings::UpdateSettings& updateSettings) = 0;
};

}  // namespace lamp::update