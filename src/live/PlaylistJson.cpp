#include "live/PlaylistJson.h"

#include <ArduinoJson.h>

namespace lamp::live {

namespace {

constexpr size_t kPlaylistJsonCapacity = 2048;

bool readRequiredString(JsonVariantConst variant, std::string& output) {
  if (!variant.is<const char*>()) {
    return false;
  }

  output = variant.as<const char*>();
  return !output.empty();
}

}  // namespace

std::string buildPlaylistJson(const PlaylistModel& playlist) {
  StaticJsonDocument<kPlaylistJsonCapacity> document;
  document["id"] = playlist.id;
  document["name"] = playlist.name;
  document["repeat"] = playlist.repeat;

  JsonArray entries = document.createNestedArray("entries");
  for (const PlaylistEntry& entry : playlist.entries) {
    JsonObject jsonEntry = entries.createNestedObject();
    jsonEntry["presetId"] = entry.presetId;
    jsonEntry["durationSec"] = entry.durationSec;
    jsonEntry["enabled"] = entry.enabled;
  }

  std::string json;
  serializeJson(document, json);
  return json;
}

bool parsePlaylistJson(const std::string& json, PlaylistModel& playlist) {
  StaticJsonDocument<kPlaylistJsonCapacity> document;
  if (deserializeJson(document, json) != DeserializationError::Ok) {
    return false;
  }

  JsonObjectConst root = document.as<JsonObjectConst>();
  PlaylistModel parsed;
  if (!readRequiredString(root["id"], parsed.id) || !readRequiredString(root["name"], parsed.name) ||
      !root["repeat"].is<bool>()) {
    return false;
  }

  parsed.repeat = root["repeat"].as<bool>();

  JsonArrayConst entries = root["entries"].as<JsonArrayConst>();
  if (entries.isNull()) {
    return false;
  }

  for (JsonObjectConst jsonEntry : entries) {
    PlaylistEntry entry;
    if (!readRequiredString(jsonEntry["presetId"], entry.presetId) ||
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

}  // namespace lamp::live