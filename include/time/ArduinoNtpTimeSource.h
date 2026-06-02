#pragma once

#include <string>

#include "time/ITimeSource.h"

namespace lamp::time {

/// Reason the NTP sync attempt resolved the way it did.
enum class NtpSyncStatus {
  kSynced,      ///< SNTP sync completed, time is valid.
  kPending,     ///< SNTP client started but has not yet synced.
  kFailed,      ///< SNTP sync timed out or failed.
  kDisabled,    ///< NTP sync is not enabled by planner.
  kCached       ///< No network sync possible, using previously cached time.
};

class ArduinoNtpTimeSource : public ITimeSource {
 public:
  bool syncTime(const char* timezone, const char* primaryServer,
                const char* secondaryServer) override;
  bool hasValidTime() const override;
  std::string formattedTime() const override;

  /// Returns the most recent sync status, useful for diagnostic endpoints.
  NtpSyncStatus lastSyncStatus() const { return lastSyncStatus_; }

  /// Returns the last successfully synced Unix epoch (0 if never synced).
  time_t lastEpoch() const { return lastEpoch_; }

 private:
  bool configured_ = false;
  bool validTime_ = false;
  unsigned long syncAttemptMs_ = 0;
  time_t lastEpoch_ = 0;
  std::string configuredTimezone_;
  std::string cachedFormattedTime_;
  NtpSyncStatus lastSyncStatus_ = NtpSyncStatus::kDisabled;
};

}  // namespace lamp::time
