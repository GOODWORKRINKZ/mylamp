#include "network/ArduinoWiFiAdapter.h"

#include <Arduino.h>
#include <WiFi.h>

#include "AppConfig.h"

namespace lamp::network {

bool ArduinoWiFiAdapter::startAccessPoint(const std::string& ssid) {
  WiFi.mode(WIFI_AP);
  WiFi.disconnect(true, true);
  delay(100);
  return WiFi.softAP(ssid.c_str(), config::kAccessPointPassword);
}

bool ArduinoWiFiAdapter::connectStation(const std::string& ssid, const std::string& password) {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false, true);
  delay(100);
  WiFi.begin(ssid.c_str(), password.c_str());

  const unsigned long startedAt = millis();
  while (millis() - startedAt < config::kWiFiConnectTimeoutMs) {
    if (WiFi.status() == WL_CONNECTED) {
      return true;
    }
    delay(config::kWiFiPollIntervalMs);
  }

  return WiFi.status() == WL_CONNECTED;
}

std::string ArduinoWiFiAdapter::localIp() const {
  const String ip = WiFi.localIP().toString();
  return ip.length() == 0 ? std::string() : std::string(ip.c_str());
}

}  // namespace lamp::network
