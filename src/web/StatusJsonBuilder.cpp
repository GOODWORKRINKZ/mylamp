#include "web/StatusJsonBuilder.h"

#include <iomanip>
#include <sstream>

namespace lamp::web {

namespace {

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

void appendField(std::string& json, const char* key, const std::string& value, bool trailingComma) {
  json += '"';
  json += key;
  json += "\":\"";
  json += escapeJson(value);
  json += '"';
  if (trailingComma) {
    json += ',';
  }
}

void appendBoolField(std::string& json, const char* key, bool value, bool trailingComma) {
  json += '"';
  json += key;
  json += "\":";
  json += value ? "true" : "false";
  if (trailingComma) {
    json += ',';
  }
}

std::string formatFloat(float value) {
  std::ostringstream stream;
  stream << std::fixed << std::setprecision(2) << value;
  std::string formatted = stream.str();

  while (!formatted.empty() && formatted.back() == '0') {
    formatted.pop_back();
  }

  if (!formatted.empty() && formatted.back() == '.') {
    formatted.pop_back();
  }

  return formatted;
}

void appendFloatField(std::string& json, const char* key, bool available, float value,
                      bool trailingComma) {
  json += '"';
  json += key;
  json += "\":";
  json += available ? formatFloat(value) : "null";
  if (trailingComma) {
    json += ',';
  }
}

}  // namespace

std::string buildStatusJson(const StatusSnapshot& snapshot) {
  std::string json = "{";
  appendField(json, "version", snapshot.version, true);
  appendField(json, "channel", snapshot.channel, true);
  appendField(json, "board", snapshot.board, true);
  appendField(json, "hardwareType", snapshot.hardwareType, true);
  appendField(json, "networkMode", snapshot.networkMode, true);
  appendField(json, "networkStatus", snapshot.networkStatus, true);
  appendField(json, "clockStatus", snapshot.clockStatus, true);
  appendField(json, "currentTime", snapshot.currentTime, true);
  appendField(json, "sensorStatus", snapshot.sensorStatus, true);
  appendFloatField(json, "temperatureC", snapshot.sensorAvailable, snapshot.temperatureC, true);
  appendFloatField(json, "humidityPercent", snapshot.sensorAvailable, snapshot.humidityPercent,
                   true);
  appendField(json, "activeEffect", snapshot.activeEffect, true);
  appendField(json, "activePresetId", snapshot.activePresetId, true);
  appendField(json, "activePresetName", snapshot.activePresetName, true);
  appendBoolField(json, "autoplayEnabled", snapshot.autoplayEnabled, true);
  appendField(json, "activePlaylistId", snapshot.activePlaylistId, true);
  appendField(json, "liveErrorSummary", snapshot.liveErrorSummary, false);
  json += '}';
  return json;
}

}  // namespace lamp::web
