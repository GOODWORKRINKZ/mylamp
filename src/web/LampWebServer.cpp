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

std::string escapeJsonValue(const std::string& input) {
  std::string output;
  output.reserve(input.size());

  for (char ch : input) {
    switch (ch) {
      case '\\':
        output += "\\\\";
        break;
      case '"':
        output += "\\\"";
        break;
      case '\n':
        output += "\\n";
        break;
      case '\r':
        output += "\\r";
        break;
      case '\t':
        output += "\\t";
        break;
      default:
        output += ch;
        break;
    }
  }

  return output;
}

std::string buildUpdateSettingsJson(const lamp::settings::AppSettings& settings) {
  std::string json = "{";
  json += "\"channel\":\"";
  json += escapeJsonValue(settings.update.channel);
  json += "\"}";
  return json;
}

std::string buildCurrentUpdateJson(const StatusSnapshot& snapshot) {
  std::string json = "{";
  json += "\"version\":\"" + escapeJsonValue(snapshot.version) + "\",";
  json += "\"channel\":\"" + escapeJsonValue(snapshot.channel) + "\",";
  json += "\"board\":\"" + escapeJsonValue(snapshot.board) + "\",";
  json += "\"hardwareType\":\"" + escapeJsonValue(snapshot.hardwareType) + "\",";
  json += "\"updateChannel\":\"" + escapeJsonValue(snapshot.updateChannel) + "\",";
  json += "\"updateState\":\"" + escapeJsonValue(snapshot.updateState) + "\",";
  json += "\"availableVersion\":\"" + escapeJsonValue(snapshot.availableVersion) + "\",";
  json += "\"updateError\":\"" + escapeJsonValue(snapshot.updateError) + "\"}";
  return json;
}

std::string buildCheckUpdatesJson(const lamp::update::FirmwareReleaseInfo& release) {
  std::string json = "{";
  json += "\"hasUpdate\":";
  json += release.available ? "true" : "false";
  json += ",\"channel\":\"" + escapeJsonValue(release.channel) + "\",";
  json += "\"version\":\"" + escapeJsonValue(release.version) + "\",";
  json += "\"assetName\":\"" + escapeJsonValue(release.assetName) + "\",";
  json += "\"downloadUrl\":\"" + escapeJsonValue(release.assetUrl) + "\",";
  json += "\"checksumUrl\":\"" + escapeJsonValue(release.checksumUrl) + "\",";
  json += "\"error\":\"" + escapeJsonValue(release.error) + "\"}";
  return json;
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

void LampWebServer::setUpdateCallbacks(UpdateChecker checker, UpdateInstaller installer) {
  checkUpdates_ = checker;
  installUpdate_ = installer;
}

void LampWebServer::registerRoutes() {
  server_.on("/", [this]() { handleRoot(); });
  server_.on("/favicon.svg", [this]() { handleFavicon(); });
  server_.on("/script.js", [this]() { handleScript(); });
  server_.on("/styles.css", [this]() { handleStyles(); });
  server_.on("/api/status", [this]() { handleStatus(); });
  server_.on("/api/settings/network", HTTP_GET, [this]() { handleGetNetworkSettings(); });
  server_.on("/api/settings/network", HTTP_POST, [this]() { handleUpdateNetworkSettings(); });
  server_.on("/api/settings/time", HTTP_GET, [this]() { handleGetTimeSettings(); });
  server_.on("/api/settings/time", HTTP_POST, [this]() { handleUpdateTimeSettings(); });
  server_.on("/api/update/current", HTTP_GET, [this]() { handleCurrentUpdate(); });
  server_.on("/api/update/check", HTTP_POST, [this]() { handleCheckUpdates(); });
  server_.on("/api/update/install", HTTP_POST, [this]() { handleInstallUpdate(); });
  server_.on("/api/update/settings", HTTP_GET, [this]() { handleGetUpdateSettings(); });
  server_.on("/api/update/settings", HTTP_POST, [this]() { handleUpdateSettings(); });
  server_.on("/api/live/validate", HTTP_POST, [this]() { handleLiveValidate(); });
  server_.on("/api/live/run", HTTP_POST, [this]() { handleLiveRun(); });
  server_.on("/api/presets", HTTP_GET, [this]() { handleListPresets(); });
  server_.on("/api/presets", HTTP_PUT, [this]() { handlePutPreset(); });
  server_.on("/api/playlists", HTTP_GET, [this]() { handleListPlaylists(); });
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

void LampWebServer::handleFavicon() {
  sendEmbeddedAsset(makeCompressedTextAsset(faviconSvg, faviconSvg_len, "image/svg+xml"));
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

void LampWebServer::handleGetTimeSettings() {
  if (!getSettings_) {
    server_.send(500, "application/json", "{\"error\":\"settings unavailable\"}");
    return;
  }

  server_.send(200, "application/json", buildTimeSettingsJson(getSettings_()).c_str());
}

void LampWebServer::handleUpdateTimeSettings() {
  if (!getSettings_ || !saveSettings_) {
    server_.send(500, "application/json", "{\"error\":\"settings unavailable\"}");
    return;
  }

  settings::AppSettings settings = getSettings_();
  if (!applyTimeSettingsUpdate(server_.arg("timezone").c_str(), settings)) {
    server_.send(400, "application/json", "{\"error\":\"invalid timezone\"}");
    return;
  }

  saveSettings_(settings);
  server_.send(200, "application/json", buildTimeSettingsJson(settings).c_str());
}

void LampWebServer::handleGetUpdateSettings() {
  if (!getSettings_) {
    server_.send(500, "application/json", "{\"error\":\"settings unavailable\"}");
    return;
  }

  server_.send(200, "application/json", buildUpdateSettingsJson(getSettings_()).c_str());
}

void LampWebServer::handleUpdateSettings() {
  if (!getSettings_ || !saveSettings_) {
    server_.send(500, "application/json", "{\"error\":\"settings unavailable\"}");
    return;
  }

  const std::string channel = server_.arg("channel").c_str();
  if (channel != "stable" && channel != "dev") {
    server_.send(400, "application/json", "{\"error\":\"invalid update channel\"}");
    return;
  }

  settings::AppSettings settings = getSettings_();
  settings.update.channel = channel;
  saveSettings_(settings);
  server_.send(200, "application/json", buildUpdateSettingsJson(settings).c_str());
}

void LampWebServer::handleCurrentUpdate() {
  server_.send(200, "application/json", buildCurrentUpdateJson(snapshot_).c_str());
}

void LampWebServer::handleCheckUpdates() {
  if (!checkUpdates_) {
    server_.send(500, "application/json", "{\"error\":\"update checker unavailable\"}");
    return;
  }

  const lamp::update::FirmwareReleaseInfo release =
      checkUpdates_(server_.arg("channel").c_str());
  server_.send(200, "application/json", buildCheckUpdatesJson(release).c_str());
}

void LampWebServer::handleInstallUpdate() {
  if (!installUpdate_) {
    server_.send(500, "application/json", "{\"error\":\"update installer unavailable\"}");
    return;
  }

  std::string error;
  const bool installed = installUpdate_(error);
  if (!installed) {
    server_.send(400, "application/json",
                 (std::string("{\"success\":false,\"error\":\"") +
                  escapeJsonValue(error) + "\"}")
                     .c_str());
    return;
  }

  server_.send(200, "application/json", "{\"success\":true,\"rebooting\":true}");
}

void LampWebServer::handleLiveValidate() {
  if (liveProgramService_ == nullptr) {
    server_.send(500, "application/json", "{\"error\":\"live runtime unavailable\"}");
    return;
  }

  const LiveApiResponse response =
      handleLiveValidateRequest(*liveProgramService_, server_.arg("plain").c_str());
  server_.send(response.statusCode, "application/json", response.body.c_str());
}

void LampWebServer::handleLiveRun() {
  if (liveProgramService_ == nullptr) {
    server_.send(500, "application/json", "{\"error\":\"live runtime unavailable\"}");
    return;
  }

  const LiveApiResponse response =
      handleLiveRunRequest(*liveProgramService_, server_.arg("plain").c_str());
  server_.send(response.statusCode, "application/json", response.body.c_str());
}

void LampWebServer::handleListPresets() {
  if (presetRepository_ == nullptr) {
    server_.send(500, "application/json", "{\"error\":\"preset repository unavailable\"}");
    return;
  }

  sendPresetApiResponse(server_, handleListPresetsRequest(*presetRepository_));
}

void LampWebServer::handlePutPreset() {
  if (presetRepository_ == nullptr) {
    server_.send(500, "application/json", "{\"error\":\"preset repository unavailable\"}");
    return;
  }

  const std::string presetId = server_.arg("id").c_str();
  if (presetId.empty()) {
    server_.send(400, "application/json", "{\"error\":\"preset id is required\"}");
    return;
  }

  sendPresetApiResponse(server_,
                        handlePutPresetRequest(*presetRepository_, presetId,
                                               server_.arg("plain").c_str()));
}

void LampWebServer::handleListPlaylists() {
  if (playlistRepository_ == nullptr) {
    server_.send(500, "application/json", "{\"error\":\"playlist repository unavailable\"}");
    return;
  }

  sendPlaylistApiResponse(server_, handleListPlaylistsRequest(*playlistRepository_));
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
