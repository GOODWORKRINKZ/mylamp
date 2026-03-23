#include "web/TimeSettingsJson.h"

#include "AppConfig.h"

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
      default:
        output += ch;
        break;
    }
  }

  return output;
}

bool isSupportedTimezone(const std::string& value) {
  static constexpr const char* kSupportedTimezones[] = {
      "UTC0",
      "EET-2EEST,M3.5.0/3,M10.5.0/4",
      "MSK-3",
      "CET-1CEST,M3.5.0,M10.5.0/3",
      "EST5EDT,M3.2.0/2,M11.1.0/2",
      "PST8PDT,M3.2.0/2,M11.1.0/2",
  };

  for (const char* supported : kSupportedTimezones) {
    if (value == supported) {
      return true;
    }
  }

  return false;
}

}  // namespace

std::string buildTimeSettingsJson(const settings::AppSettings& settings) {
  return std::string("{\"timezone\":\"") + escapeJson(settings.clock.timezone) + "\"}";
}

bool applyTimeSettingsUpdate(const std::string& timezone, settings::AppSettings& settings) {
  if (!isSupportedTimezone(timezone)) {
    return false;
  }

  settings.clock.timezone = timezone.empty() ? lamp::config::kTimeZone : timezone;
  return true;
}

}  // namespace lamp::web