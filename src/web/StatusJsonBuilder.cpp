#include "web/StatusJsonBuilder.h"

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

}  // namespace

std::string buildStatusJson(const StatusSnapshot& snapshot) {
  std::string json = "{";
  appendField(json, "version", snapshot.version, true);
  appendField(json, "channel", snapshot.channel, true);
  appendField(json, "board", snapshot.board, true);
  appendField(json, "networkMode", snapshot.networkMode, true);
  appendField(json, "networkStatus", snapshot.networkStatus, true);
  appendField(json, "clockStatus", snapshot.clockStatus, true);
  appendField(json, "activeEffect", snapshot.activeEffect, false);
  json += '}';
  return json;
}

}  // namespace lamp::web
