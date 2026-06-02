#include "web/TimeStatusJson.h"

#include <cstdlib>
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
      default:
        output += ch;
        break;
    }
  }

  return output;
}

}  // namespace

std::string buildTimeStatusJson(const TimeStatusSnapshot& snapshot) {
  std::ostringstream json;
  json << "{";
  json << "\"currentTime\":\"" << escapeJson(snapshot.currentTime) << "\",";
  json << "\"timezone\":\"" << escapeJson(snapshot.timezone) << "\",";
  json << "\"syncStatus\":\"" << escapeJson(snapshot.syncStatus) << "\",";
  json << "\"ntpServer\":\"" << escapeJson(snapshot.ntpServer) << "\",";
  json << "\"epoch\":" << snapshot.epoch;
  json << "}";
  return json.str();
}

}  // namespace lamp::web
