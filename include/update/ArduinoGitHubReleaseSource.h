#pragma once

#include <string>

#include "update/GitHubReleaseParser.h"
#include "update/IFirmwareReleaseSource.h"

namespace lamp::update {

class ArduinoGitHubReleaseSource final : public IFirmwareReleaseSource {
 public:
  explicit ArduinoGitHubReleaseSource(std::string githubRepo);

  FirmwareReleaseInfo check(const BuildIdentity& buildIdentity,
                            const settings::UpdateSettings& updateSettings) override;

 private:
  std::string githubRepo_;
  GitHubReleaseParser parser_;
};

}  // namespace lamp::update