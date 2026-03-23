#include "time/ArduinoNtpTimeSource.h"

#include <time.h>

#include <Arduino.h>

namespace lamp::time {

bool ArduinoNtpTimeSource::syncTime(const char* timezone, const char* primaryServer,
                                    const char* secondaryServer) {
  if (!configured_) {
    configTzTime(timezone, primaryServer, secondaryServer);
    configured_ = true;
  }

  tm timeinfo{};
  if (!getLocalTime(&timeinfo, 1000)) {
    return validTime_;
  }

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
