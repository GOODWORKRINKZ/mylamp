#pragma once

#include <string>

namespace lamp::time {

class ITimeSource {
 public:
  virtual ~ITimeSource() = default;

  virtual bool syncTime(const char* timezone, const char* primaryServer,
                        const char* secondaryServer) = 0;
  virtual bool hasValidTime() const = 0;
  virtual std::string formattedTime() const = 0;
};

}  // namespace lamp::time
