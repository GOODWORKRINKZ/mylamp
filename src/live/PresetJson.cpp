#include "live/PresetJson.h"

#include <ArduinoJson.h>

namespace lamp::live {

namespace {

constexpr size_t kPresetJsonCapacity = 2048;

bool readRequiredString(JsonVariantConst variant, std::string& output) {
  if (!variant.is<const char*>()) {
    return false;
  }

  output = variant.as<const char*>();
  return !output.empty();
}

}  // namespace

std::string buildPresetJson(const PresetModel& preset) {
  StaticJsonDocument<kPresetJsonCapacity> document;
  document["id"] = preset.id;
  document["name"] = preset.name;
  document["source"] = preset.source;
  document["createdAt"] = preset.createdAt;
  document["updatedAt"] = preset.updatedAt;

  JsonArray tags = document.createNestedArray("tags");
  for (const std::string& tag : preset.tags) {
    tags.add(tag);
  }

  JsonObject options = document.createNestedObject("options");
  options["brightnessCap"] = preset.options.brightnessCap;

  std::string json;
  serializeJson(document, json);
  return json;
}

bool parsePresetJson(const std::string& json, PresetModel& preset) {
  StaticJsonDocument<kPresetJsonCapacity> document;
  if (deserializeJson(document, json) != DeserializationError::Ok) {
    return false;
  }

  JsonObjectConst root = document.as<JsonObjectConst>();
  PresetModel parsed;
  if (!readRequiredString(root["id"], parsed.id) || !readRequiredString(root["name"], parsed.name) ||
      !readRequiredString(root["source"], parsed.source)) {
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

}  // namespace lamp::live