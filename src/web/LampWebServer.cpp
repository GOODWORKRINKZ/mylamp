#include "web/LampWebServer.h"

#include <pgmspace.h>

#include "embedded_resources.h"

namespace lamp::web {

namespace {

lamp::live::Diagnostic makeDiagnostic(uint32_t line, uint32_t column,
                                     const std::string& message) {
  lamp::live::Diagnostic diagnostic;
  diagnostic.line = line;
  diagnostic.column = column;
  diagnostic.message = message;
  return diagnostic;
}

std::string buildNotImplementedLiveResponse(const std::string& fallbackMessage) {
  std::vector<lamp::live::Diagnostic> diagnostics;
  diagnostics.push_back(makeDiagnostic(0U, 0U, fallbackMessage));
  return lamp::live::buildDiagnosticResponseJson(false, diagnostics);
}

}  // namespace

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
  server_.on("/api/live/validate", HTTP_POST, [this]() { handleLiveValidate(); });
  server_.on("/api/live/run", HTTP_POST, [this]() { handleLiveRun(); });
}

void LampWebServer::handleRoot() {
  sendEmbeddedAsset(
      makeCompressedTextAsset(indexHtml, indexHtml_len, "text/html; charset=utf-8"));
}

void LampWebServer::handleScript() {
  sendEmbeddedAsset(
      makeCompressedTextAsset(scriptJs, scriptJs_len, "application/javascript; charset=utf-8"));
}

void LampWebServer::handleStyles() {
  sendEmbeddedAsset(
      makeCompressedTextAsset(stylesCss, stylesCss_len, "text/css; charset=utf-8"));
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

void LampWebServer::handleLiveValidate() {
  lamp::live::LiveRequest request;
  if (!lamp::live::parseLiveRequestJson(server_.arg("plain").c_str(), request)) {
    std::vector<lamp::live::Diagnostic> diagnostics;
    diagnostics.push_back(makeDiagnostic(0U, 0U, "Некорректный JSON запроса"));
    server_.send(400, "application/json",
                 lamp::live::buildDiagnosticResponseJson(false, diagnostics).c_str());
    return;
  }

  server_.send(200, "application/json",
               buildNotImplementedLiveResponse("Проверка DSL пока не реализована").c_str());
}

void LampWebServer::handleLiveRun() {
  lamp::live::LiveRequest request;
  if (!lamp::live::parseLiveRequestJson(server_.arg("plain").c_str(), request)) {
    std::vector<lamp::live::Diagnostic> diagnostics;
    diagnostics.push_back(makeDiagnostic(0U, 0U, "Некорректный JSON запроса"));
    server_.send(400, "application/json",
                 lamp::live::buildDiagnosticResponseJson(false, diagnostics).c_str());
    return;
  }

  server_.send(200, "application/json",
               buildNotImplementedLiveResponse("Запуск DSL пока не реализован").c_str());
}

void LampWebServer::sendEmbeddedAsset(const EmbeddedAsset& asset) {
  if (isCompressedAsset(asset)) {
    server_.sendHeader("Content-Encoding", asset.contentEncoding);
  }

  server_.send_P(200, asset.contentType, reinterpret_cast<PGM_P>(asset.data), asset.length);
}

}  // namespace lamp::web
