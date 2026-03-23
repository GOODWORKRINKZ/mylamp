#include "web/PresetApi.h"

#include <ArduinoJson.h>

#include "live/PresetJson.h"

namespace lamp::web {

namespace {

constexpr size_t kPresetRequestCapacity = 2048;
constexpr size_t kPresetListCapacity = 4096;

PresetApiResponse makeJsonResponse(int statusCode, const std::string& body) {
  PresetApiResponse response;
  response.statusCode = statusCode;
  response.body = body;
  return response;
}

PresetApiResponse makeErrorResponse(int statusCode, const char* message) {
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

bool parsePresetUpsertBody(const std::string& presetId, const std::string& body,
                          lamp::live::PresetModel& preset) {
  StaticJsonDocument<kPresetRequestCapacity> document;
  if (deserializeJson(document, body) != DeserializationError::Ok) {
    return false;
  }

  JsonObjectConst root = document.as<JsonObjectConst>();
  lamp::live::PresetModel parsed;
  parsed.id = presetId;
  if (!readRequiredString(root, "name", parsed.name) || !readRequiredString(root, "source", parsed.source)) {
    return false;
  }

  if (root.containsKey("createdAt") && !root["createdAt"].is<const char*>()) {
    return false;
  }
  if (root.containsKey("updatedAt") && !root["updatedAt"].is<const char*>()) {
    return false;
  }
  parsed.createdAt = root["createdAt"] | "";
  parsed.updatedAt = root["updatedAt"] | "";

  JsonArrayConst tags = root["tags"].as<JsonArrayConst>();
  if (!tags.isNull()) {
    for (JsonVariantConst tag : tags) {
      if (!tag.is<const char*>()) {
        return false;
      }
      parsed.tags.push_back(tag.as<const char*>());
    }
  }

  JsonObjectConst options = root["options"].as<JsonObjectConst>();
  if (!options.isNull()) {
    if (options.containsKey("brightnessCap") && !options["brightnessCap"].is<float>() &&
        !options["brightnessCap"].is<double>() && !options["brightnessCap"].is<int>()) {
      return false;
    }
    parsed.options.brightnessCap = options["brightnessCap"] | 1.0f;
  }

  preset = parsed;
  return true;
}

std::string buildPresetListJson(const std::vector<lamp::live::PresetModel>& presets) {
  StaticJsonDocument<kPresetListCapacity> document;
  JsonArray items = document.createNestedArray("items");
  for (const lamp::live::PresetModel& preset : presets) {
    JsonObject item = items.createNestedObject();
    item["id"] = preset.id;
    item["name"] = preset.name;
    item["updatedAt"] = preset.updatedAt;
  }

  std::string json;
  serializeJson(document, json);
  return json;
}

std::string buildPresetActivationJson(const lamp::live::runtime::LiveProgramState& state) {
  StaticJsonDocument<256> document;
  document["ok"] = state.active;
  document["activePresetId"] = state.activePresetId;
  document["temporary"] = state.temporary;
  document["autoplayActive"] = state.autoplayActive;

  std::string json;
  serializeJson(document, json);
  return json;
}

}  // namespace

PresetApiResponse handleListPresetsRequest(const lamp::live::PresetRepository& repository) {
  if (!repository.isReady()) {
    return makeErrorResponse(503, "storage unavailable");
  }

  return makeJsonResponse(200, buildPresetListJson(repository.list()));
}

PresetApiResponse handleGetPresetRequest(const lamp::live::PresetRepository& repository,
                                         const std::string& presetId) {
  if (!repository.isReady()) {
    return makeErrorResponse(503, "storage unavailable");
  }

  lamp::live::PresetModel preset;
  if (!repository.load(presetId, preset)) {
    return makeErrorResponse(404, "preset not found");
  }

  return makeJsonResponse(200, lamp::live::buildPresetJson(preset));
}

PresetApiResponse handlePutPresetRequest(lamp::live::PresetRepository& repository,
                                         const std::string& presetId,
                                         const std::string& body) {
  if (!repository.isReady()) {
    return makeErrorResponse(503, "storage unavailable");
  }

  lamp::live::PresetModel preset;
  if (!parsePresetUpsertBody(presetId, body, preset)) {
    return makeErrorResponse(400, "invalid preset payload");
  }
  if (!repository.save(preset)) {
    return makeErrorResponse(500, "failed to save preset");
  }

  return makeJsonResponse(200, lamp::live::buildPresetJson(preset));
}

PresetApiResponse handleDeletePresetRequest(lamp::live::PresetRepository& repository,
                                            const std::string& presetId) {
  if (!repository.isReady()) {
    return makeErrorResponse(503, "storage unavailable");
  }

  if (!repository.remove(presetId)) {
    return makeErrorResponse(404, "preset not found");
  }

  return makeJsonResponse(200, "{\"ok\":true}");
}

PresetApiResponse handleActivatePresetRequest(
    const lamp::live::PresetRepository& repository,
    lamp::live::runtime::LiveProgramService& runtimeService, const std::string& presetId) {
  if (!repository.isReady()) {
    return makeErrorResponse(503, "storage unavailable");
  }

  lamp::live::PresetModel preset;
  if (!repository.load(presetId, preset)) {
    return makeErrorResponse(404, "preset not found");
  }

  std::vector<lamp::live::Diagnostic> diagnostics;
  if (!runtimeService.activatePreset(preset, diagnostics)) {
    return makeErrorResponse(400, "preset source failed to compile");
  }

  return makeJsonResponse(200, buildPresetActivationJson(runtimeService.state()));
}

}  // namespace lamp::web