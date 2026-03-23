#pragma once

#include "update/IFirmwareInstaller.h"

namespace lamp::update {

class Esp32FirmwareInstaller final : public IFirmwareInstaller {
 public:
  bool install(const FirmwareReleaseInfo& release, std::string& error) override;
};

}  // namespace lamp::update