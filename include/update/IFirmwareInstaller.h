#pragma once

#include <string>

#include "update/FirmwareReleaseInfo.h"

namespace lamp::update {

class IFirmwareInstaller {
 public:
  virtual ~IFirmwareInstaller() = default;

  virtual bool install(const FirmwareReleaseInfo& release, std::string& error) = 0;
};

}  // namespace lamp::update