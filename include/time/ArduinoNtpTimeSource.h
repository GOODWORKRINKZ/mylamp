#pragma once

#include <string>

#include "time/ITimeSource.h"

namespace lamp::time {

class ArduinoNtpTimeSource : public ITimeSource {
 public:
  bool syncTime(const char* timezone, const char* primaryServer,
                const char* secondaryServer) override;
  bool hasValidTime() const override;
  std::string formattedTime() const override;

 private:
  bool configured_ = false;
  bool validTime_ = false;
  std::string configuredTimezone_;
  std::string cachedFormattedTime_;
};

}  // namespace lamp::time
