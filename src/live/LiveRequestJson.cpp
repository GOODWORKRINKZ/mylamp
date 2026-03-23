#include "live/LiveRequestJson.h"

#include <ArduinoJson.h>

namespace lamp::live {

namespace {

constexpr size_t kLiveRequestJsonCapacity = 2048;

std::string escapeJson(const std::string& input) {
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
      default:
        output += ch;
        break;
    }
  }

  return output;
}

}  // namespace

bool parseLiveRequestJson(const std::string& json, LiveRequest& request) {
  StaticJsonDocument<kLiveRequestJsonCapacity> document;
  if (deserializeJson(document, json) != DeserializationError::Ok) {
    return false;
  }

  JsonObjectConst root = document.as<JsonObjectConst>();
  if (!root["source"].is<const char*>()) {
    return false;
  }

  LiveRequest parsed;
  parsed.source = root["source"].as<const char*>();
  if (parsed.source.empty()) {
    return false;
  }

  if (root.containsKey("presetName")) {
    if (!root["presetName"].is<const char*>()) {
      return false;
    }
    parsed.presetName = root["presetName"].as<const char*>();
  }

  request = parsed;
  return true;
}

std::string buildDiagnosticResponseJson(bool ok, const std::vector<Diagnostic>& diagnostics) {
  std::string json = "{";
  json += std::string("\"ok\":") + (ok ? "true" : "false") + ",";
  json += "\"errors\":[";

  for (size_t index = 0; index < diagnostics.size(); ++index) {
    const Diagnostic& diagnostic = diagnostics[index];
    json += "{";
    json += "\"line\":" + std::to_string(diagnostic.line) + ",";
    json += "\"column\":" + std::to_string(diagnostic.column) + ",";
    json += "\"message\":\"" + escapeJson(diagnostic.message) + "\"";
    json += "}";
    if (index + 1 < diagnostics.size()) {
      json += ",";
    }
  }

  json += "]}";
  return json;
}

}  // namespace lamp::live