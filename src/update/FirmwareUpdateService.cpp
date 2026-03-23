#include "update/FirmwareUpdateService.h"

namespace lamp::update {

const char* firmwareUpdateStateToString(FirmwareUpdateState state) {
  switch (state) {
    case FirmwareUpdateState::kIdle:
      return "idle";
    case FirmwareUpdateState::kChecking:
      return "checking";
    case FirmwareUpdateState::kUpToDate:
      return "up-to-date";
    case FirmwareUpdateState::kAvailable:
      return "available";
    case FirmwareUpdateState::kInstalling:
      return "installing";
    case FirmwareUpdateState::kCompleted:
      return "completed";
    case FirmwareUpdateState::kError:
      return "error";
  }

  return "unknown";
}

FirmwareUpdateService::FirmwareUpdateService(BuildIdentity buildIdentity,
                                             IFirmwareReleaseSource& releaseSource,
                                             IFirmwareInstaller& installer)
    : buildIdentity_(std::move(buildIdentity)),
      releaseSource_(releaseSource),
      installer_(installer) {}

const BuildIdentity& FirmwareUpdateService::buildIdentity() const {
  return buildIdentity_;
}

const FirmwareUpdateStatus& FirmwareUpdateService::status() const {
  return status_;
}

const FirmwareReleaseInfo& FirmwareUpdateService::check(
    const settings::UpdateSettings& updateSettings) {
  status_.state = FirmwareUpdateState::kChecking;
  status_.error.clear();

  status_.lastRelease = releaseSource_.check(buildIdentity_, updateSettings);
  if (status_.lastRelease.available) {
    status_.state = FirmwareUpdateState::kAvailable;
    return status_.lastRelease;
  }

  status_.error = status_.lastRelease.error;
  status_.state = status_.error.empty() ? FirmwareUpdateState::kUpToDate : FirmwareUpdateState::kError;
  return status_.lastRelease;
}

bool FirmwareUpdateService::install(std::string& error) {
  error.clear();
  status_.error.clear();

  if (!status_.lastRelease.available) {
    error = "no-available-release";
    status_.error = error;
    status_.state = FirmwareUpdateState::kError;
    return false;
  }

  status_.state = FirmwareUpdateState::kInstalling;
  if (!installer_.install(status_.lastRelease, error)) {
    status_.error = error;
    status_.state = FirmwareUpdateState::kError;
    return false;
  }

  status_.error.clear();
  status_.state = FirmwareUpdateState::kCompleted;
  return true;
}

}  // namespace lamp::update