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

bool startsWith(const std::string& value, const char* prefix) {
  return value.rfind(prefix, 0) == 0;
}

std::string trimPresetPath(const std::string& uri) {
  static constexpr const char* kPrefix = "/api/presets/";
  return uri.substr(std::char_traits<char>::length(kPrefix));
}

std::string trimPlaylistPath(const std::string& uri) {
  static constexpr const char* kPrefix = "/api/playlists/";
  return uri.substr(std::char_traits<char>::length(kPrefix));
}

void sendPresetApiResponse(WebServer& server, const PresetApiResponse& response) {
  server.send(response.statusCode, "application/json", response.body.c_str());
}

void sendPlaylistApiResponse(WebServer& server, const PlaylistApiResponse& response) {
  server.send(response.statusCode, "application/json", response.body.c_str());
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

void LampWebServer::setPresetServices(live::PresetRepository* repository,
                                      live::runtime::LiveProgramService* runtimeService) {
  presetRepository_ = repository;
  liveProgramService_ = runtimeService;
}

void LampWebServer::setPlaylistServices(live::PlaylistRepository* playlistRepository,
                                        live::PresetRepository* presetRepository,
                                        live::runtime::PlaylistScheduler* scheduler,
                                        live::runtime::LiveProgramService* runtimeService) {
  playlistRepository_ = playlistRepository;
  presetRepository_ = presetRepository;
  playlistScheduler_ = scheduler;
  liveProgramService_ = runtimeService;
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
  server_.on("/api/presets", HTTP_GET, [this]() { handleListPresets(); });
  server_.on("/api/playlists/stop", HTTP_POST, [this]() { handlePlaylistByPath(); });
  server_.onNotFound([this]() { handleNotFound(); });
}

void LampWebServer::handleNotFound() {
  const std::string uri = server_.uri().c_str();
  if (startsWith(uri, "/api/presets/")) {
    handlePresetByPath();
    return;
  }
  if (startsWith(uri, "/api/playlists/")) {
    handlePlaylistByPath();
    return;
  }

  server_.send(404, "application/json", "{\"error\":\"not found\"}");
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

void LampWebServer::handleListPresets() {
  if (presetRepository_ == nullptr) {
    server_.send(500, "application/json", "{\"error\":\"preset repository unavailable\"}");
    return;
  }

  sendPresetApiResponse(server_, handleListPresetsRequest(*presetRepository_));
}

void LampWebServer::handlePresetByPath() {
  if (presetRepository_ == nullptr) {
    server_.send(500, "application/json", "{\"error\":\"preset repository unavailable\"}");
    return;
  }

  const std::string suffix = trimPresetPath(server_.uri().c_str());
  if (suffix.empty()) {
    server_.send(404, "application/json", "{\"error\":\"not found\"}");
    return;
  }

  const std::string activateSuffix = "/activate";
  if (suffix.size() > activateSuffix.size() &&
      suffix.compare(suffix.size() - activateSuffix.size(), activateSuffix.size(), activateSuffix) == 0) {
    if (liveProgramService_ == nullptr) {
      server_.send(500, "application/json", "{\"error\":\"live runtime unavailable\"}");
      return;
    }

    const std::string presetId = suffix.substr(0, suffix.size() - activateSuffix.size());
    if (server_.method() == HTTP_POST) {
      sendPresetApiResponse(server_,
                            handleActivatePresetRequest(*presetRepository_, *liveProgramService_, presetId));
      return;
    }
  } else {
    if (server_.method() == HTTP_GET) {
      sendPresetApiResponse(server_, handleGetPresetRequest(*presetRepository_, suffix));
      return;
    }
    if (server_.method() == HTTP_PUT) {
      sendPresetApiResponse(server_,
                            handlePutPresetRequest(*presetRepository_, suffix, server_.arg("plain").c_str()));
      return;
    }
    if (server_.method() == HTTP_DELETE) {
      sendPresetApiResponse(server_, handleDeletePresetRequest(*presetRepository_, suffix));
      return;
    }
  }

  server_.send(404, "application/json", "{\"error\":\"not found\"}");
}

void LampWebServer::handlePlaylistByPath() {
  if (playlistRepository_ == nullptr || presetRepository_ == nullptr || playlistScheduler_ == nullptr ||
      liveProgramService_ == nullptr) {
    server_.send(500, "application/json", "{\"error\":\"playlist services unavailable\"}");
    return;
  }

  const std::string uri = server_.uri().c_str();
  if (uri == "/api/playlists/stop" && server_.method() == HTTP_POST) {
    sendPlaylistApiResponse(server_, handleStopPlaylistRequest(*playlistScheduler_, *liveProgramService_));
    return;
  }

  const std::string suffix = trimPlaylistPath(uri);
  const std::string startSuffix = "/start";
  if (suffix.size() > startSuffix.size() &&
      suffix.compare(suffix.size() - startSuffix.size(), startSuffix.size(), startSuffix) == 0) {
    const std::string playlistId = suffix.substr(0, suffix.size() - startSuffix.size());
    if (server_.method() == HTTP_POST) {
      std::vector<lamp::live::Diagnostic> diagnostics;
      sendPlaylistApiResponse(server_, handleStartPlaylistRequest(
                                           *playlistRepository_, *presetRepository_, *playlistScheduler_,
                                           *liveProgramService_, playlistId, diagnostics));
      return;
    }
  } else {
    if (server_.method() == HTTP_PUT) {
      sendPlaylistApiResponse(server_,
                              handlePutPlaylistRequest(*playlistRepository_, suffix,
                                                       server_.arg("plain").c_str()));
      return;
    }
    if (server_.method() == HTTP_DELETE) {
      sendPlaylistApiResponse(server_, handleDeletePlaylistRequest(*playlistRepository_, suffix));
      return;
    }
  }

  server_.send(404, "application/json", "{\"error\":\"not found\"}");
}

void LampWebServer::sendEmbeddedAsset(const EmbeddedAsset& asset) {
  if (isCompressedAsset(asset)) {
    server_.sendHeader("Content-Encoding", asset.contentEncoding);
  }

  server_.send_P(200, asset.contentType, reinterpret_cast<PGM_P>(asset.data), asset.length);
}

}  // namespace lamp::web
