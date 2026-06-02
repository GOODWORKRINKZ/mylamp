#include "time/ArduinoNtpTimeSource.h"

#include <time.h>

#include <Arduino.h>

namespace lamp::time {

bool ArduinoNtpTimeSource::syncTime(const char* timezone, const char* primaryServer,
                                    const char* secondaryServer) {
  const std::string requestedTimezone = timezone ? timezone : "";
  if (!configured_ || requestedTimezone != configuredTimezone_) {
    configTzTime(timezone, primaryServer, secondaryServer);
    configured_ = true;
    configuredTimezone_ = requestedTimezone;
  }

  time_t now_epoch;
  ::time(&now_epoch);
  if (now_epoch < 1700000000) {
    return validTime_;
  }

  tm timeinfo{};
  localtime_r(&now_epoch, &timeinfo);

  char buffer[16];
  if (strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo) == 0) {
    return validTime_;
  }

  cachedFormattedTime_ = buffer;
  validTime_ = true;
  return true;
}

bool ArduinoNtpTimeSource::hasValidTime() const {
  return validTime_;
}

std::string ArduinoNtpTimeSource::formattedTime() const {
  return cachedFormattedTime_;
}

}  // namespace lamp::time
