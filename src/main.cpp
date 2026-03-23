#include <Arduino.h>

#include "AppConfig.h"
#include "BuildInfo.h"
#include "FrameBuffer.h"
#include "MatrixLayout.h"
#include "effects/AlternatingColumnsEffect.h"
#include "effects/EffectContext.h"
#include "effects/EffectRegistry.h"
#include "effects/SolidColorEffect.h"
#include "network/ArduinoWiFiAdapter.h"
#include "network/NetworkPlanner.h"
#include "network/WiFiManager.h"
#include "sensors/ArduinoAht30SensorSource.h"
#include "sensors/ISensorSource.h"
#include "sensors/SensorRuntimeService.h"
#include "settings/AppSettings.h"
#include "settings/AppSettingsPersistence.h"
#include "settings/PreferencesSettingsBackend.h"
#include "time/ArduinoNtpTimeSource.h"
#include "time/ITimeSource.h"
#include "time/TimePlanner.h"
#include "time/TimeRuntimeService.h"
#include "web/LampWebServer.h"
#include "web/StatusJsonBuilder.h"

namespace {

lamp::MatrixLayout g_layout;
lamp::FrameBuffer g_frameBuffer(g_layout);
lamp::effects::SolidColorEffect g_bootEffect(lamp::Rgb{0, 0, 24}, "boot-solid");
lamp::effects::AlternatingColumnsEffect g_patternEffect(
  lamp::Rgb{10, 0, 0}, lamp::Rgb{0, 10, 0}, "debug-columns");
lamp::effects::EffectRegistry g_effectRegistry;
lamp::settings::AppSettings g_settings;
lamp::settings::AppSettingsPersistence g_settingsPersistence;
lamp::settings::PreferencesSettingsBackend g_settingsBackend;
lamp::network::ArduinoWiFiAdapter g_wifiAdapter;
lamp::network::WiFiManager g_wifiManager;
lamp::network::NetworkPlanner g_networkPlanner;
lamp::network::PlannedNetworkState g_networkState;
lamp::time::ArduinoNtpTimeSource g_timeSource;
lamp::time::TimePlanner g_timePlanner;
lamp::time::PlannedTimeState g_timeState;
lamp::time::TimeRuntimeService g_timeRuntimeService;
lamp::time::RuntimeTimeState g_runtimeTimeState;
lamp::sensors::ArduinoAht30SensorSource g_sensorSource;
lamp::sensors::SensorRuntimeService g_sensorRuntimeService;
lamp::sensors::RuntimeSensorState g_sensorState;
lamp::web::LampWebServer g_webServer;
unsigned long g_lastHeartbeatMs = 0;
unsigned long g_lastTimeRefreshMs = 0;
unsigned long g_lastSensorRefreshMs = 0;
bool g_usePatternEffect = false;
bool g_networkReconfigureRequested = false;

lamp::web::StatusSnapshot buildStatusSnapshot();

void refreshSensorState() {
  g_sensorState = g_sensorRuntimeService.refresh(g_sensorState, g_sensorSource);
}

void refreshRuntimeState(const lamp::network::WiFiStartupResult& wifiResult) {
  const bool clientActive = wifiResult.activeMode == lamp::network::NetworkMode::kClient;
  g_networkState = g_networkPlanner.planStartup(
      g_settings.network, clientActive, clientActive, wifiResult.ipAddress);
  g_timeState = g_timePlanner.plan(g_settings.clock, g_networkState, g_timeSource.hasValidTime());
  g_runtimeTimeState = g_timeRuntimeService.refresh(g_timeState, g_timeSource);
  g_webServer.setStatusSnapshot(buildStatusSnapshot());
}

lamp::web::StatusSnapshot buildStatusSnapshot() {
  lamp::web::StatusSnapshot snapshot;
  snapshot.version = lamp::BuildInfo::version;
  snapshot.channel = lamp::BuildInfo::channel;
  snapshot.board = lamp::BuildInfo::board;
  snapshot.networkMode =
      g_networkState.activeMode == lamp::network::NetworkMode::kClient ? "client" : "ap";
  snapshot.networkStatus = g_networkState.statusLine;
  snapshot.clockStatus = g_timeState.statusLine;
  snapshot.currentTime = g_runtimeTimeState.currentTime;
  snapshot.sensorStatus = g_sensorState.statusLine;
  snapshot.sensorAvailable = g_sensorState.available;
  snapshot.temperatureC = g_sensorState.temperatureC;
  snapshot.humidityPercent = g_sensorState.humidityPercent;
  if (const lamp::effects::IEffect* effect = g_effectRegistry.active()) {
    snapshot.activeEffect = effect->name();
  }
  return snapshot;
}

lamp::settings::AppSettings getCurrentSettings() {
  return g_settings;
}

void saveAndApplySettings(const lamp::settings::AppSettings& settings) {
  g_settings = settings;
  g_settingsPersistence.save(g_settings, g_settingsBackend);
  g_networkReconfigureRequested = true;
}

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
  Serial.print("current time: ");
  Serial.println(g_runtimeTimeState.currentTime.c_str());
  Serial.print("sensor state: ");
  Serial.println(g_sensorState.statusLine.c_str());
  if (g_sensorState.available) {
    Serial.print("temperature C: ");
    Serial.println(g_sensorState.temperatureC);
    Serial.print("humidity %: ");
    Serial.println(g_sensorState.humidityPercent);
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(200);
  g_settings = g_settingsPersistence.load(g_settingsBackend);
    g_webServer.setSettingsCallbacks(getCurrentSettings, saveAndApplySettings);
  const lamp::network::WiFiStartupResult wifiResult =
      g_wifiManager.startup(g_settings.network, g_wifiAdapter);
  refreshRuntimeState(wifiResult);
  g_effectRegistry.add(g_bootEffect);
  g_effectRegistry.add(g_patternEffect);
  refreshSensorState();
  g_webServer.setStatusSnapshot(buildStatusSnapshot());
  g_webServer.begin();
  lamp::effects::EffectContext context{0, g_frameBuffer};
  g_effectRegistry.renderActive(context);
  printBootBanner();
}

void loop() {
  g_webServer.loop();
  if (g_networkReconfigureRequested) {
    g_networkReconfigureRequested = false;
    const lamp::network::WiFiStartupResult wifiResult =
        g_wifiManager.startup(g_settings.network, g_wifiAdapter);
    refreshRuntimeState(wifiResult);
  }
  const unsigned long now = millis();
  if (now - g_lastTimeRefreshMs >= lamp::config::kTimeRefreshIntervalMs) {
    g_lastTimeRefreshMs = now;
    g_runtimeTimeState = g_timeRuntimeService.refresh(g_timeState, g_timeSource);
    g_webServer.setStatusSnapshot(buildStatusSnapshot());
  }
  if (now - g_lastSensorRefreshMs >= lamp::config::kSensorRefreshIntervalMs) {
    g_lastSensorRefreshMs = now;
    refreshSensorState();
    g_webServer.setStatusSnapshot(buildStatusSnapshot());
  }
  if (now - g_lastHeartbeatMs >= 5000UL) {
    g_lastHeartbeatMs = now;
    g_usePatternEffect = !g_usePatternEffect;
    g_effectRegistry.setActiveByName(g_usePatternEffect ? "debug-columns" : "boot-solid");
    g_webServer.setStatusSnapshot(buildStatusSnapshot());
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
    Serial.print("sensor state: ");
    Serial.println(g_sensorState.statusLine.c_str());
  }
}
