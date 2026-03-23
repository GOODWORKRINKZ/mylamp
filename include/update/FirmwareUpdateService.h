#pragma once

#include <string>

#include "update/FirmwareReleaseInfo.h"
#include "update/IFirmwareInstaller.h"
#include "update/IFirmwareReleaseSource.h"

namespace lamp::update {

enum class FirmwareUpdateState {
  kIdle,
  kChecking,
  kUpToDate,
  kAvailable,
  kInstalling,
  kCompleted,
  kError,
};

const char* firmwareUpdateStateToString(FirmwareUpdateState state);

struct FirmwareUpdateStatus {
  FirmwareUpdateState state = FirmwareUpdateState::kIdle;
  FirmwareReleaseInfo lastRelease;
  std::string error;
};

class FirmwareUpdateService {
 public:
  FirmwareUpdateService(BuildIdentity buildIdentity, IFirmwareReleaseSource& releaseSource,
                        IFirmwareInstaller& installer);

  const BuildIdentity& buildIdentity() const;
  const FirmwareUpdateStatus& status() const;

  const FirmwareReleaseInfo& check(const settings::UpdateSettings& updateSettings);
  bool install(std::string& error);

 private:
  BuildIdentity buildIdentity_;
  IFirmwareReleaseSource& releaseSource_;
  IFirmwareInstaller& installer_;
  FirmwareUpdateStatus status_;
};

}  // namespace lamp::update