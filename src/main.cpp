#include <Arduino.h>
#include <LittleFS.h>

#include "AppConfig.h"
#include "BuildInfo.h"
#include "FrameBuffer.h"
#include "MatrixLayout.h"
#include "effects/AlternatingColumnsEffect.h"
#include "effects/EffectContext.h"
#include "effects/EffectRegistry.h"
#include "effects/SolidColorEffect.h"
#include "live/PlaylistRepository.h"
#include "live/runtime/LiveProgramService.h"
#include "live/runtime/PlaylistScheduler.h"
#include "network/ArduinoWiFiAdapter.h"
#include "network/NetworkPlanner.h"
#include "network/WiFiManager.h"
#include "live/runtime/RuntimeContext.h"
#include "live/PresetRepository.h"
#include "sensors/ArduinoAht30SensorSource.h"
#include "sensors/ISensorSource.h"
#include "sensors/SensorRuntimeService.h"
#include "settings/AppSettings.h"
#include "settings/AppSettingsPersistence.h"
#include "settings/PreferencesSettingsBackend.h"
#include "storage/ContentPaths.h"
#include "storage/LittleFsFileStore.h"
#include "storage/StorageBootstrap.h"
#include "time/ArduinoNtpTimeSource.h"
#include "time/ITimeSource.h"
#include "time/TimePlanner.h"
#include "time/TimeRuntimeService.h"
#include "update/ArduinoGitHubReleaseSource.h"
#include "update/Esp32FirmwareInstaller.h"
#include "update/FirmwareUpdateService.h"
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
lamp::storage::LittleFsFileStore g_fileStore(LittleFS);
lamp::live::PresetRepository g_presetRepository(g_fileStore);
lamp::live::PlaylistRepository g_playlistRepository(g_fileStore);
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
lamp::update::ArduinoGitHubReleaseSource g_releaseSource(lamp::BuildInfo::githubRepo);
lamp::update::Esp32FirmwareInstaller g_firmwareInstaller;
lamp::update::FirmwareUpdateService g_updateService(
  lamp::update::BuildIdentity{"mylamp", lamp::BuildInfo::version, lamp::BuildInfo::channel,
                lamp::BuildInfo::board, lamp::BuildInfo::hardwareType},
  g_releaseSource, g_firmwareInstaller);
lamp::live::runtime::LiveProgramService g_liveProgramService;
lamp::live::runtime::PlaylistScheduler g_playlistScheduler;
lamp::web::LampWebServer g_webServer;
unsigned long g_lastHeartbeatMs = 0;
unsigned long g_lastTimeRefreshMs = 0;
unsigned long g_lastSensorRefreshMs = 0;
unsigned long g_lastPlaylistTickMs = 0;
unsigned long g_lastRenderMs = 0;
bool g_usePatternEffect = false;
bool g_networkReconfigureRequested = false;
bool g_fileSystemReady = false;
bool g_restartRequested = false;
std::string g_liveErrorSummary;

lamp::web::StatusSnapshot buildStatusSnapshot();
lamp::update::FirmwareReleaseInfo checkForFirmwareUpdates(const std::string& channelOverride);
bool installFirmwareUpdate(std::string& error);

void renderFrame(unsigned long nowMs) {
  const unsigned long deltaMs = g_lastRenderMs == 0 ? 0 : nowMs - g_lastRenderMs;
  g_lastRenderMs = nowMs;

  lamp::live::runtime::RuntimeContext runtimeContext;
  runtimeContext.nowMs = static_cast<uint32_t>(nowMs);
  runtimeContext.deltaMs = static_cast<uint32_t>(deltaMs);
  runtimeContext.temperatureC = g_sensorState.temperatureC;
  runtimeContext.humidityPercent = g_sensorState.humidityPercent;

  if (g_liveProgramService.render(runtimeContext, g_frameBuffer)) {
    return;
  }

  lamp::effects::EffectContext effectContext{static_cast<uint32_t>(nowMs), g_frameBuffer};
  g_effectRegistry.renderActive(effectContext);
}

void initializeFileSystem() {
  g_fileSystemReady = LittleFS.begin(true);
  if (!g_fileSystemReady) {
    Serial.println("filesystem: mount failed");
    return;
  }

  if (!lamp::storage::ensureContentDirectories(LittleFS)) {
    Serial.println("filesystem: failed to prepare content directories");
    g_fileSystemReady = false;
    return;
  }

  const std::vector<std::string> presets = g_fileStore.list(lamp::storage::kPresetsDirectory);
  const std::vector<std::string> playlists = g_fileStore.list(lamp::storage::kPlaylistsDirectory);
  Serial.print("filesystem: mounted presets=");
  Serial.print(static_cast<unsigned long>(presets.size()));
  Serial.print(" playlists=");
  Serial.println(static_cast<unsigned long>(playlists.size()));
}

void refreshSensorState() {
  g_sensorState = g_sensorRuntimeService.refresh(g_sensorState, g_sensorSource);
}

void refreshRuntimeState(const lamp::network::WiFiStartupResult& wifiResult) {
  const bool clientActive = wifiResult.activeMode == lamp::network::NetworkMode::kClient;
  g_networkState = g_networkPlanner.planStartup(
      g_settings.network, clientActive, clientActive, wifiResult.ipAddress);
  g_timeState = g_timePlanner.plan(g_settings.clock, g_networkState, g_timeSource.hasValidTime());
  g_runtimeTimeState = g_timeRuntimeService.refresh(g_settings.clock, g_timeState, g_timeSource);
  g_webServer.setStatusSnapshot(buildStatusSnapshot());
}

lamp::web::StatusSnapshot buildStatusSnapshot() {
  lamp::web::StatusSnapshot snapshot;
  snapshot.version = lamp::BuildInfo::version;
  snapshot.channel = lamp::BuildInfo::channel;
  snapshot.board = lamp::BuildInfo::board;
  snapshot.hardwareType = lamp::BuildInfo::hardwareType;
  snapshot.updateChannel = g_settings.update.channel;
  snapshot.updateState = lamp::update::firmwareUpdateStateToString(g_updateService.status().state);
  snapshot.availableVersion = g_updateService.status().lastRelease.available
                                  ? g_updateService.status().lastRelease.version
                                  : std::string();
  snapshot.updateError = g_updateService.status().error;
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
  const lamp::live::runtime::LiveProgramState liveState = g_liveProgramService.state();
  snapshot.activePresetId = liveState.activePresetId;
  if (!snapshot.activePresetId.empty()) {
    lamp::live::PresetModel preset;
    if (g_presetRepository.load(snapshot.activePresetId, preset)) {
      snapshot.activePresetName = preset.name;
    }
  }
  snapshot.autoplayEnabled = liveState.autoplayActive;
  snapshot.activePlaylistId = g_playlistScheduler.state().activePlaylistId;
  snapshot.liveErrorSummary = g_liveErrorSummary;
  return snapshot;
}

lamp::settings::AppSettings getCurrentSettings() {
  return g_settings;
}

void saveAndApplySettings(const lamp::settings::AppSettings& settings) {
  const bool networkChanged = settings.network.preferredMode != g_settings.network.preferredMode ||
                              settings.network.accessPointName != g_settings.network.accessPointName ||
                              settings.network.clientSsid != g_settings.network.clientSsid ||
                              settings.network.clientPassword != g_settings.network.clientPassword;
  const bool clockChanged = settings.clock.enabled != g_settings.clock.enabled ||
                            settings.clock.showCachedTimeWhenOffline != g_settings.clock.showCachedTimeWhenOffline ||
                            settings.clock.timezone != g_settings.clock.timezone;
  g_settings = settings;
  g_settingsPersistence.save(g_settings, g_settingsBackend);
  g_timeState = g_timePlanner.plan(g_settings.clock, g_networkState, g_timeSource.hasValidTime());
  g_runtimeTimeState = g_timeRuntimeService.refresh(g_settings.clock, g_timeState, g_timeSource);
  g_webServer.setStatusSnapshot(buildStatusSnapshot());
  g_networkReconfigureRequested = networkChanged;
  (void)clockChanged;
}

lamp::update::FirmwareReleaseInfo checkForFirmwareUpdates(const std::string& channelOverride) {
  lamp::settings::UpdateSettings updateSettings = g_settings.update;
  if (!channelOverride.empty()) {
    updateSettings.channel = channelOverride;
  }

  const lamp::update::FirmwareReleaseInfo& release = g_updateService.check(updateSettings);
  g_webServer.setStatusSnapshot(buildStatusSnapshot());
  return release;
}

bool installFirmwareUpdate(std::string& error) {
  const bool installed = g_updateService.install(error);
  g_webServer.setStatusSnapshot(buildStatusSnapshot());
  if (installed) {
    g_restartRequested = true;
  }
  return installed;
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
  Serial.print("hardware type: ");
  Serial.println(lamp::BuildInfo::hardwareType);
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
  initializeFileSystem();
  g_settings = g_settingsPersistence.load(g_settingsBackend);
  g_webServer.setSettingsCallbacks(getCurrentSettings, saveAndApplySettings);
  g_webServer.setUpdateCallbacks(checkForFirmwareUpdates, installFirmwareUpdate);
  g_webServer.setPresetServices(&g_presetRepository, &g_liveProgramService);
  g_webServer.setPlaylistServices(&g_playlistRepository, &g_presetRepository, &g_playlistScheduler,
                                  &g_liveProgramService);
  const lamp::network::WiFiStartupResult wifiResult =
      g_wifiManager.startup(g_settings.network, g_wifiAdapter);
  refreshRuntimeState(wifiResult);
  g_effectRegistry.add(g_bootEffect);
  g_effectRegistry.add(g_patternEffect);
  refreshSensorState();
  g_webServer.setStatusSnapshot(buildStatusSnapshot());
  g_webServer.begin();
  renderFrame(0);
  printBootBanner();
}

void loop() {
  g_webServer.loop();
  if (g_restartRequested) {
    delay(250);
    ESP.restart();
  }
  if (g_networkReconfigureRequested) {
    g_networkReconfigureRequested = false;
    const lamp::network::WiFiStartupResult wifiResult =
        g_wifiManager.startup(g_settings.network, g_wifiAdapter);
    refreshRuntimeState(wifiResult);
  }
  const unsigned long now = millis();
  const unsigned long playlistDeltaMs = g_lastPlaylistTickMs == 0 ? 0 : now - g_lastPlaylistTickMs;
  g_lastPlaylistTickMs = now;
  g_playlistScheduler.syncWithRuntime(g_liveProgramService);
  std::vector<lamp::live::Diagnostic> playlistDiagnostics;
  g_playlistScheduler.advance(static_cast<uint32_t>(playlistDeltaMs), g_presetRepository,
                              g_liveProgramService, playlistDiagnostics);
  if (now - g_lastTimeRefreshMs >= lamp::config::kTimeRefreshIntervalMs) {
    g_lastTimeRefreshMs = now;
    g_runtimeTimeState = g_timeRuntimeService.refresh(g_settings.clock, g_timeState, g_timeSource);
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
    renderFrame(now);
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
  renderFrame(now);
}
