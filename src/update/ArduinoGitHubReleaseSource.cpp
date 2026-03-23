#include "update/ArduinoGitHubReleaseSource.h"

#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

namespace lamp::update {

extern const uint8_t rootca_crt_bundle_start[] asm("_binary_x509_crt_bundle_start");

namespace {

std::string buildReleaseUrl(const std::string& githubRepo, const std::string& channel) {
  const std::string baseUrl = "https://api.github.com/repos/" + githubRepo + "/releases";
  return channel == "dev" ? baseUrl + "?per_page=10" : baseUrl + "/latest";
}

}  // namespace

ArduinoGitHubReleaseSource::ArduinoGitHubReleaseSource(std::string githubRepo)
    : githubRepo_(std::move(githubRepo)) {}

FirmwareReleaseInfo ArduinoGitHubReleaseSource::check(
    const BuildIdentity& buildIdentity, const settings::UpdateSettings& updateSettings) {
  FirmwareReleaseInfo info;
  info.channel = updateSettings.channel;

  if (WiFi.status() != WL_CONNECTED) {
    info.error = "wifi-disconnected";
    return info;
  }

  WiFiClientSecure client;
  client.setCACertBundle(rootca_crt_bundle_start);

  HTTPClient http;
  const std::string url = buildReleaseUrl(githubRepo_, updateSettings.channel);
  if (!http.begin(client, url.c_str())) {
    info.error = "http-begin-failed";
    return info;
  }

  http.addHeader("Accept", "application/vnd.github+json");
  http.addHeader("User-Agent", "mylamp-firmware-updater");

  const int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    info.error = "http-" + std::to_string(httpCode);
    http.end();
    return info;
  }

  const String payload = http.getString();
  http.end();

  info = parser_.parse(payload.c_str(), buildIdentity.version, updateSettings.channel,
                       buildIdentity.hardwareType);
  if (info.channel.empty()) {
    info.channel = updateSettings.channel;
  }
  return info;
}

}  // namespace lamp::update