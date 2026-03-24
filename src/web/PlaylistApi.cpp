#include "web/PlaylistApi.h"

#include <ArduinoJson.h>

#include "live/PlaylistJson.h"

namespace lamp::web {

namespace {

constexpr size_t kPlaylistRequestCapacity = 2048;

PlaylistApiResponse makeJsonResponse(int statusCode, const std::string& body) {
  PlaylistApiResponse response;
  response.statusCode = statusCode;
  response.body = body;
  return response;
}

PlaylistApiResponse makeErrorResponse(int statusCode, const char* message) {
  StaticJsonDocument<256> document;
  document["error"] = message;
  std::string body;
  serializeJson(document, body);
  return makeJsonResponse(statusCode, body);
}

bool readRequiredString(JsonObjectConst root, const char* key, std::string& output) {
  if (!root[key].is<const char*>()) {
    return false;
  }
  output = root[key].as<const char*>();
  return !output.empty();
}

bool parsePlaylistUpsertBody(const std::string& playlistId, const std::string& body,
                            lamp::live::PlaylistModel& playlist) {
  StaticJsonDocument<kPlaylistRequestCapacity> document;
  if (deserializeJson(document, body) != DeserializationError::Ok) {
    return false;
  }

  JsonObjectConst root = document.as<JsonObjectConst>();
  lamp::live::PlaylistModel parsed;
  parsed.id = playlistId;
  if (!readRequiredString(root, "name", parsed.name) || !root["repeat"].is<bool>()) {
    return false;
  }
  parsed.repeat = root["repeat"].as<bool>();

  JsonArrayConst entries = root["entries"].as<JsonArrayConst>();
  if (entries.isNull()) {
    return false;
  }

  for (JsonObjectConst jsonEntry : entries) {
    lamp::live::PlaylistEntry entry;
    if (!readRequiredString(jsonEntry, "presetId", entry.presetId) ||
        !jsonEntry["durationSec"].is<uint32_t>()) {
      return false;
    }
    entry.durationSec = jsonEntry["durationSec"].as<uint32_t>();
    entry.enabled = jsonEntry["enabled"] | true;
    parsed.entries.push_back(entry);
  }

  playlist = parsed;
  return true;
}

std::string buildPlaylistStateJson(const lamp::live::runtime::PlaylistSchedulerState& state) {
  StaticJsonDocument<256> document;
  document["ok"] = true;
  document["activePlaylistId"] = state.activePlaylistId;
  document["active"] = state.active;
  document["autoplayActive"] = state.autoplayActive;
  document["activeEntryIndex"] = state.activeEntryIndex;

  std::string body;
  serializeJson(document, body);
  return body;
}

}  // namespace

PlaylistApiResponse handlePutPlaylistRequest(lamp::live::PlaylistRepository& repository,
                                             const std::string& playlistId,
                                             const std::string& body) {
  if (!repository.isReady()) {
    return makeErrorResponse(503, "storage unavailable");
  }

  lamp::live::PlaylistModel playlist;
  if (!parsePlaylistUpsertBody(playlistId, body, playlist)) {
    return makeErrorResponse(400, "invalid playlist payload");
  }
  if (!repository.save(playlist)) {
    return makeErrorResponse(500, "failed to save playlist");
  }
  return makeJsonResponse(200, lamp::live::buildPlaylistJson(playlist));
}

PlaylistApiResponse handleDeletePlaylistRequest(lamp::live::PlaylistRepository& repository,
                                                const std::string& playlistId) {
  if (!repository.isReady()) {
    return makeErrorResponse(503, "storage unavailable");
  }

  if (!repository.remove(playlistId)) {
    return makeErrorResponse(404, "playlist not found");
  }
  return makeJsonResponse(200, "{\"ok\":true}");
}

PlaylistApiResponse handleStartPlaylistRequest(
    const lamp::live::PlaylistRepository& playlistRepository,
    const lamp::live::PresetRepository& presetRepository,
    lamp::live::runtime::PlaylistScheduler& scheduler,
    lamp::live::runtime::LiveProgramService& runtimeService, const std::string& playlistId,
    std::vector<lamp::live::Diagnostic>& diagnostics) {
  if (!playlistRepository.isReady() || !presetRepository.isReady()) {
    return makeErrorResponse(503, "storage unavailable");
  }

  lamp::live::PlaylistModel playlist;
  if (!playlistRepository.load(playlistId, playlist)) {
    return makeErrorResponse(404, "playlist not found");
  }
  if (!scheduler.start(playlist, presetRepository, runtimeService, diagnostics)) {
    return makeErrorResponse(400, "failed to start playlist");
  }
  return makeJsonResponse(200, buildPlaylistStateJson(scheduler.state()));
}

PlaylistApiResponse handleStopPlaylistRequest(lamp::live::runtime::PlaylistScheduler& scheduler,
                                              lamp::live::runtime::LiveProgramService& runtimeService) {
  runtimeService.setAutoplayActive(false);
  scheduler.stop();
  return makeJsonResponse(200, "{\"ok\":true}");
}

}  // namespace lamp::web