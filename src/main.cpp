#include <Arduino.h>

#include "AppConfig.h"
#include "BuildInfo.h"
#include "MatrixLayout.h"

namespace {

lamp::MatrixLayout g_layout;
unsigned long g_lastHeartbeatMs = 0;

void printBootBanner() {
  Serial.println();
  Serial.println("mylamp bootstrap");
  Serial.print("board: ");
  Serial.println(lamp::BuildInfo::board);
  Serial.print("version: ");
  Serial.println(lamp::BuildInfo::version);
  Serial.print("channel: ");
  Serial.println(lamp::BuildInfo::channel);
  Serial.print("logical canvas: ");
  Serial.print(lamp::config::kLogicalWidth);
  Serial.print("x");
  Serial.println(lamp::config::kLogicalHeight);
  Serial.print("sample pixel map x=0 y=0 -> ");
  Serial.println(g_layout.toLinearIndex(0, 0));
  Serial.print("sample pixel map x=31 y=15 -> ");
  Serial.println(g_layout.toLinearIndex(31, 15));
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(200);
  printBootBanner();
}

void loop() {
  const unsigned long now = millis();
  if (now - g_lastHeartbeatMs >= 5000UL) {
    g_lastHeartbeatMs = now;
    Serial.print("heartbeat uptime_ms=");
    Serial.println(now);
  }
}
