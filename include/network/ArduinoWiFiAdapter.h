#pragma once

#include <string>

#include "network/IWiFiAdapter.h"

namespace lamp::network {

class ArduinoWiFiAdapter : public IWiFiAdapter {
 public:
  bool startAccessPoint(const std::string& ssid) override;
  bool connectStation(const std::string& ssid, const std::string& password) override;
  std::string localIp() const override;
};

}  // namespace lamp::network
