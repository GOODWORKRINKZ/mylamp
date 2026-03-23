#include "web/LampWebServer.h"

#include <pgmspace.h>

#include "embedded_resources.h"

namespace lamp::web {

LampWebServer::LampWebServer() : server_(80) {}

void LampWebServer::begin() {
  registerRoutes();
  server_.begin();
}

void LampWebServer::loop() {
  server_.handleClient();
}

void LampWebServer::setStatusSnapshot(StatusSnapshot snapshot) {
  snapshot_ = snapshot;
}

void LampWebServer::setSettingsCallbacks(SettingsGetter getter, SettingsSaver saver) {
  getSettings_ = getter;
  saveSettings_ = saver;
}

void LampWebServer::registerRoutes() {
  server_.on("/", [this]() { handleRoot(); });
  server_.on("/script.js", [this]() { handleScript(); });
  server_.on("/styles.css", [this]() { handleStyles(); });
  server_.on("/api/status", [this]() { handleStatus(); });
  server_.on("/api/settings/network", HTTP_GET, [this]() { handleGetNetworkSettings(); });
  server_.on("/api/settings/network", HTTP_POST, [this]() { handleUpdateNetworkSettings(); });
}

void LampWebServer::handleRoot() {
  sendEmbeddedAsset(indexHtml, indexHtml_len, "text/html; charset=utf-8");
}

void LampWebServer::handleScript() {
  sendEmbeddedAsset(scriptJs, scriptJs_len, "application/javascript; charset=utf-8");
}

void LampWebServer::handleStyles() {
  sendEmbeddedAsset(stylesCss, stylesCss_len, "text/css; charset=utf-8");
}

void LampWebServer::handleStatus() {
  server_.send(200, "application/json", buildStatusJson(snapshot_).c_str());
}

void LampWebServer::handleGetNetworkSettings() {
  if (!getSettings_) {
    server_.send(500, "application/json", "{\"error\":\"settings unavailable\"}");
    return;
  }

  server_.send(200, "application/json", buildNetworkSettingsJson(getSettings_()).c_str());
}

void LampWebServer::handleUpdateNetworkSettings() {
  if (!getSettings_ || !saveSettings_) {
    server_.send(500, "application/json", "{\"error\":\"settings unavailable\"}");
    return;
  }

  settings::AppSettings settings = getSettings_();
  const bool applied = applyNetworkSettingsUpdate(
      server_.arg("mode").c_str(), server_.arg("accessPointName").c_str(),
      server_.arg("clientSsid").c_str(), server_.arg("clientPassword").c_str(), settings);
  if (!applied) {
    server_.send(400, "application/json", "{\"error\":\"invalid mode\"}");
    return;
  }

  saveSettings_(settings);
  server_.send(200, "application/json", buildNetworkSettingsJson(settings).c_str());
}

void LampWebServer::sendEmbeddedAsset(const uint8_t* data, size_t length,
                                      const char* contentType) {
  server_.send_P(200, contentType, reinterpret_cast<PGM_P>(data), length);
}

}  // namespace lamp::web
