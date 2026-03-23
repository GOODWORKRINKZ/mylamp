#pragma once

#include <string>

namespace lamp::network {

class IWiFiAdapter {
 public:
  virtual ~IWiFiAdapter() = default;

  virtual bool startAccessPoint(const std::string& ssid) = 0;
  virtual bool connectStation(const std::string& ssid, const std::string& password) = 0;
  virtual std::string localIp() const = 0;
};

}  // namespace lamp::network
