#include "time/ArduinoNtpTimeSource.h"

#include <time.h>

#include <Arduino.h>
#include <HTTPClient.h>

namespace lamp::time {

namespace {

/// Parse HTTP Date header like "Tue, 02 Jun 2026 09:32:41 GMT" → Unix epoch.
/// Returns 0 on failure.
time_t parseHttpDate(const char* dateStr) {
  if (!dateStr || !*dateStr) return 0;

  const char* p = dateStr;
  while (*p && *p != ',') ++p;
  if (*p == ',') ++p;
  while (*p == ' ') ++p;

  char month[4] = {};
  int day, year, hour, min, sec;
  if (sscanf(p, "%d %3s %d %d:%d:%d",
             &day, month, &year, &hour, &min, &sec) != 6) {
    return 0;
  }

  static const char* kMonthNames[] = {
    "Jan","Feb","Mar","Apr","May","Jun",
    "Jul","Aug","Sep","Oct","Nov","Dec"
  };
  int mon = -1;
  for (int i = 0; i < 12; ++i) {
    if (strcasecmp(month, kMonthNames[i]) == 0) { mon = i; break; }
  }
  if (mon < 0) return 0;

  // Manual UTC tm → epoch (timegm not available on all toolchains).
  static const int kDaysBeforeMonth[12] = {
    0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
  };
  int y = year;
  int leapDays = (y - 1969) / 4 - (y - 1901) / 100 + (y - 1601) / 400;
  int monthDays = kDaysBeforeMonth[mon];
  if (mon > 1 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0)) {
    monthDays++;
  }
  long days = (y - 1970L) * 365L + leapDays + monthDays + (day - 1);
  return days * 86400L + hour * 3600L + min * 60 + sec;
}

/// Parse POSIX TZ like "MSK-3" → GMT offset in seconds (positive = ahead of UTC).
long parseGmtOffsetSeconds(const char* tz) {
  if (!tz || !*tz) return 0;
  const char* p = tz;
  while (*p && ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z'))) ++p;
  if (*p == '-' || (*p >= '0' && *p <= '9')) {
    const int sign = (*p == '-') ? -1 : 1;
    if (*p == '-') ++p;
    int hours = 0;
    while (*p >= '0' && *p <= '9') { hours = hours * 10 + (*p - '0'); ++p; }
    return -sign * hours * 3600;
  }
  return 0;
}

/// Fetch current time via HTTP Date header. Returns Unix epoch or 0.
time_t fetchHttpTime(const char* url) {
  HTTPClient http;
  http.setConnectTimeout(1500);
  http.setTimeout(1500);
  const char* kHeaderKeys[] = {"Date", "date"};
  http.collectHeaders(kHeaderKeys, 2);

  if (!http.begin(url)) return 0;

  int code = http.GET();
  if (code <= 0) { http.end(); return 0; }

  String dateVal = http.header("Date");
  if (dateVal.isEmpty()) dateVal = http.header("date");

  // Some servers (e.g. cloudflare) don't expose Date via header() — search the body.
  if (dateVal.isEmpty()) {
    String body = http.getString();
    int pos = body.indexOf("Date: ");
    if (pos >= 0) {
      int end = body.indexOf("\r\n", pos);
      if (end > pos) dateVal = body.substring(pos + 6, end);
    }
  }
  http.end();

  if (dateVal.isEmpty()) return 0;
  return parseHttpDate(dateVal.c_str());
}

}  // namespace

bool ArduinoNtpTimeSource::syncTime(const char* timezone, const char* primaryServer,
                                    const char* secondaryServer) {
  (void)primaryServer;    // HTTP-based sync does not use NTP servers
  (void)secondaryServer;

  const std::string requestedTimezone = timezone ? timezone : "";

  if (!configured_ || requestedTimezone != configuredTimezone_) {
    configured_ = true;
    configuredTimezone_ = requestedTimezone;
    lastSyncStatus_ = NtpSyncStatus::kPending;
  }

  // Skip re-sync if we already have valid time within the last 60 seconds.
  if (validTime_ && syncAttemptMs_ > 0 && millis() - syncAttemptMs_ < 60000UL) {
    return true;
  }

  syncAttemptMs_ = millis();

  // Cycle through known-good HTTP servers that return a Date header.
  static const char* kTimeServers[] = {
    "http://example.com",
    "http://httpforever.com",
  };
  static constexpr int kServerCount = sizeof(kTimeServers) / sizeof(kTimeServers[0]);
  static int serverIdx = 0;

  time_t epoch = fetchHttpTime(kTimeServers[serverIdx]);
  serverIdx = (serverIdx + 1) % kServerCount;

  if (epoch == 0) {
    lastSyncStatus_ = NtpSyncStatus::kFailed;
    return validTime_;
  }

  // Apply timezone offset.
  epoch += parseGmtOffsetSeconds(configuredTimezone_.c_str());

  tm timeinfo{};
  if (gmtime_r(&epoch, &timeinfo) == nullptr) {
    lastSyncStatus_ = NtpSyncStatus::kFailed;
    return validTime_;
  }

  char buffer[16];
  if (strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo) == 0) {
    lastSyncStatus_ = NtpSyncStatus::kFailed;
    return validTime_;
  }

  cachedFormattedTime_ = buffer;
  validTime_ = true;
  lastEpoch_ = epoch;
  lastSyncStatus_ = NtpSyncStatus::kSynced;
  return true;
}

bool ArduinoNtpTimeSource::hasValidTime() const { return validTime_; }
std::string ArduinoNtpTimeSource::formattedTime() const { return cachedFormattedTime_; }

}  // namespace lamp::time
