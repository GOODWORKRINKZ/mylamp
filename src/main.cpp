#include <Arduino.h>

#include "AppConfig.h"
#include "BuildInfo.h"
#include "FrameBuffer.h"
#include "MatrixLayout.h"
#include "network/ArduinoWiFiAdapter.h"
#include "network/NetworkPlanner.h"
#include "network/WiFiManager.h"
#include "settings/AppSettings.h"
#include "time/TimePlanner.h"
#include "effects/AlternatingColumnsEffect.h"
#include "effects/EffectContext.h"
#include "effects/EffectRegistry.h"
#include "effects/SolidColorEffect.h"

namespace {

lamp::MatrixLayout g_layout;
lamp::FrameBuffer g_frameBuffer(g_layout);
lamp::effects::SolidColorEffect g_bootEffect(lamp::Rgb{0, 0, 24}, "boot-solid");
lamp::effects::AlternatingColumnsEffect g_patternEffect(
  lamp::Rgb{10, 0, 0}, lamp::Rgb{0, 10, 0}, "debug-columns");
lamp::effects::EffectRegistry g_effectRegistry;
lamp::settings::AppSettings g_settings;
lamp::network::ArduinoWiFiAdapter g_wifiAdapter;
lamp::network::WiFiManager g_wifiManager;
lamp::network::NetworkPlanner g_networkPlanner;
lamp::network::PlannedNetworkState g_networkState;
lamp::time::TimePlanner g_timePlanner;
lamp::time::PlannedTimeState g_timeState;
unsigned long g_lastHeartbeatMs = 0;
bool g_usePatternEffect = false;

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
  const lamp::Rgb bootPixel = g_frameBuffer.pixelAtIndex(g_layout.toLinearIndex(0, 0));
  Serial.print("boot pixel r=");
  Serial.print(bootPixel.r);
  Serial.print(" g=");
  Serial.print(bootPixel.g);
  Serial.print(" b=");
  Serial.println(bootPixel.b);
  if (const lamp::effects::IEffect* effect = g_effectRegistry.active()) {
    Serial.print("active effect: ");
    Serial.println(effect->name());
  }
  Serial.print("network mode: ");
  Serial.println(g_networkState.activeMode == lamp::network::NetworkMode::kClient ? "client" : "ap");
  Serial.print("network status: ");
  Serial.println(g_networkState.statusLine.c_str());
  Serial.print("time sync: ");
  Serial.println(g_networkState.timeSyncAllowed ? "enabled" : "disabled");
  Serial.print("clock state: ");
  Serial.println(g_timeState.statusLine.c_str());
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(200);
  g_settings.network.preferredMode = lamp::network::NetworkMode::kAccessPoint;
  g_settings.network.accessPointName = lamp::config::kAccessPointPrefix;
  const lamp::network::WiFiStartupResult wifiResult =
      g_wifiManager.startup(g_settings.network, g_wifiAdapter);
  const bool internetAvailable = wifiResult.activeMode == lamp::network::NetworkMode::kClient;
  g_networkState = g_networkPlanner.planStartup(
      g_settings.network, wifiResult.activeMode == lamp::network::NetworkMode::kClient,
      internetAvailable, wifiResult.ipAddress);
  g_timeState = g_timePlanner.plan(g_settings.clock, g_networkState, false);
  g_effectRegistry.add(g_bootEffect);
  g_effectRegistry.add(g_patternEffect);
  lamp::effects::EffectContext context{0, g_frameBuffer};
  g_effectRegistry.renderActive(context);
  printBootBanner();
}

void loop() {
  const unsigned long now = millis();
  if (now - g_lastHeartbeatMs >= 5000UL) {
    g_lastHeartbeatMs = now;
    g_usePatternEffect = !g_usePatternEffect;
    g_effectRegistry.setActiveByName(g_usePatternEffect ? "debug-columns" : "boot-solid");
    lamp::effects::EffectContext context{now, g_frameBuffer};
    g_effectRegistry.renderActive(context);
    Serial.print("heartbeat uptime_ms=");
    Serial.println(now);
    if (const lamp::effects::IEffect* effect = g_effectRegistry.active()) {
      Serial.print("switched effect: ");
      Serial.println(effect->name());
    }
    Serial.print("network status: ");
    Serial.println(g_networkState.statusLine.c_str());
    Serial.print("clock state: ");
    Serial.println(g_timeState.statusLine.c_str());
  }
}
