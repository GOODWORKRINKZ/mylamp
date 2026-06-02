#include <Arduino.h>
#include <FastLED.h>
#include <LittleFS.h>

#include "AppConfig.h"
#include "BuildInfo.h"
#include "FrameBuffer.h"
#include "MatrixLayout.h"
#include "effects/AlternatingColumnsEffect.h"
#include "effects/ClockOverlay.h"
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
#include "settings/SettingsAccess.h"
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
#include "web/TimeStatusJson.h"

namespace {

constexpr const char* kFireplaceSource =
  "effect \"fireplace\"\n"
  "\n"
  "sprite px {\n"
  "  bitmap \"\"\"\n"
  "##\n"
  "  \"\"\"\n"
  "}\n"
  "\n"
  "layer g {\n"
  "  use px\n"
  "  color hsv(t * 10 + sin(y * 0.2) * 40 + x * 4, 0.9, 0.12 + sin(t * 0.6 + x * 0.3) * 0.08)\n"
  "  x = 0\n"
  "  y = 0\n"
  "  scale = 16\n"
  "  visible = 1\n"
  "}\n";

lamp::MatrixLayout g_layout;
lamp::FrameBuffer g_frameBuffer(g_layout);
lamp::effects::SolidColorEffect g_bootEffect(lamp::Rgb{40, 15, 0}, "boot-solid");
lamp::effects::ClockOverlay g_clockOverlay;
lamp::effects::EffectRegistry g_effectRegistry;
lamp::settings::AppSettings g_settings;
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
CRGB g_leds[lamp::config::kPixelCount];
unsigned long g_lastHeartbeatMs = 0;
unsigned long g_lastFpsReportMs = 0;
unsigned long g_frameCount = 0;
unsigned long g_lastLoopUs = 0;
unsigned long g_lastTimeRefreshMs = 0;
unsigned long g_lastSensorRefreshMs = 0;
unsigned long g_lastPlaylistTickMs = 0;
unsigned long g_lastRenderMs = 0;
bool g_networkReconfigureRequested = false;
bool g_fileSystemReady = false;
bool g_restartRequested = false;
bool g_wasLiveActive = false;
std::string g_liveErrorSummary;

// Frame timing ring buffer for min/max/avg diagnostics.
static constexpr size_t kFrameTimeWindow = 64;
uint32_t g_frameTimesUs[kFrameTimeWindow] = {};
size_t g_frameTimeIndex = 0;
size_t g_frameTimeFilled = 0;

lamp::web::StatusSnapshot buildStatusSnapshot();
lamp::update::FirmwareReleaseInfo checkForFirmwareUpdates(const std::string& channelOverride);
bool installFirmwareUpdate(std::string& error);

void renderEffectPass(unsigned long nowMs) {
  const unsigned long deltaMs = g_lastRenderMs == 0 ? 0 : nowMs - g_lastRenderMs;
  g_lastRenderMs = nowMs;

  lamp::live::runtime::RuntimeContext runtimeContext;
  runtimeContext.nowMs = static_cast<uint32_t>(nowMs);
  runtimeContext.deltaMs = static_cast<uint32_t>(deltaMs);
  runtimeContext.temperatureC = g_sensorState.temperatureC;
  runtimeContext.humidityPercent = g_sensorState.humidityPercent;

  const bool liveRendered = g_liveProgramService.render(runtimeContext, g_frameBuffer);

  if (liveRendered) {
    g_wasLiveActive = true;
    return;
  }

  // Detect live→compiled transition: clear FB so stale DSL pixels
  // don't persist under a compiled effect that may not fill every pixel.
  if (g_wasLiveActive) {
    g_frameBuffer.clear();
    g_wasLiveActive = false;
  }

  lamp::effects::EffectContext effectContext{static_cast<uint32_t>(nowMs), g_frameBuffer};
  g_effectRegistry.renderActive(effectContext);
}

void renderOverlayPass(unsigned long nowMs) {
  g_clockOverlay.render(g_runtimeTimeState.currentTime, g_frameBuffer,
                        g_timeState.clockOverlayVisible,
                        static_cast<uint32_t>(nowMs),
                        g_sensorState.temperatureC, g_sensorState.humidityPercent,
                        g_sensorState.available);
}

void commitFrame() {
  for (uint16_t i = 0; i < lamp::config::kPixelCount; ++i) {
    const lamp::Rgb src = g_frameBuffer.pixelAtIndex(i);
    g_leds[i] = CRGB(src.r, src.g, src.b);
  }
  FastLED.show();
}

void renderFrame(unsigned long nowMs) {
  renderEffectPass(nowMs);
  renderOverlayPass(nowMs);
  commitFrame();
}

void seedFactoryPresets();

void initializeFileSystem() {
  g_fileStore.setReady(false);
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

  g_fileStore.setReady(true);

  const std::vector<std::string> presets = g_fileStore.list(lamp::storage::kPresetsDirectory);
  const std::vector<std::string> playlists = g_fileStore.list(lamp::storage::kPlaylistsDirectory);

  // Seed factory presets if /presets/ is empty (first boot)
  if (presets.empty()) {
    seedFactoryPresets();
  }

#if APP_IS_DEV
  Serial.print("filesystem: mounted presets=");
  Serial.print(static_cast<unsigned long>(presets.size()));
  Serial.print(" playlists=");
  Serial.println(static_cast<unsigned long>(playlists.size()));
#endif
}

void seedFactoryPresets() {
  if (!g_fileStore.isReady()) return;
  const std::vector<std::string> existing = g_fileStore.list(lamp::storage::kPresetsDirectory);
  if (!existing.empty()) return;

  struct DemoEntry { const char* id; const char* name; const char* source; };
  const DemoEntry demos[] = {
    {"nyan-cat", "Нян Кот",
     "effect \"nyan_cat\"\n"
     "sprite cat {\n"
     "  frame walk1 { bitmap \"\"\"\n.....##....\n....####...\n...######..\n..########.\n.##########\n##.##.##.##\n############\n.##......##\n.##########\n..########.\n\"\"\" }\n"
     "  frame walk2 { bitmap \"\"\"\n.....##....\n....####...\n...######..\n..########.\n.##########\n##.##.##.##\n############\n.##########\n.##......##\n..########.\n\"\"\" }\n"
     "  frame walk3 { bitmap \"\"\"\n.....##....\n....####...\n...######..\n..########.\n.##########\n##.##.##.##\n############\n..########.\n.##########\n.##......##\n\"\"\" }\n"
     "  frame walk4 { bitmap \"\"\"\n.....##....\n....####...\n...######..\n..########.\n.##########\n##.##.##.##\n############\n....######.\n..########.\n.##......##\n\"\"\" }\n"
     "}\n"
     "layer bg {\n"
     "  use cat\n"
     "  color hsv(nx * 30 + t * 50 + ny * 10, 1, 0.5 + 0.3 * sin(t + nx * 2))\n"
     "  x = (t * 8) % 32\n  y = 3\n  scale = 1\n  frame = (t * 4) % 4\n  visible = 1\n"
     "}\n"},
    {"mario", "Марио",
     "effect \"mario\"\n"
     "sprite mario {\n"
     "  frame walk1 { bitmap \"\"\"\n....##....\n...####...\n...####...\n....##....\n...####...\n..######..\n.##.##.##.\n.##.##.##.\n....##....\n...##.##..\n..##..##..\n\"\"\" }\n"
     "  frame walk2 { bitmap \"\"\"\n....##....\n...####...\n...####...\n....##....\n...####...\n..######..\n.##.##.##.\n.##.##.##.\n....##....\n...##.##..\n.##....##.\n\"\"\" }\n"
     "  frame walk3 { bitmap \"\"\"\n....##....\n...####...\n...####...\n....##....\n...####...\n..######..\n.##.##.##.\n.##.##.##.\n....##....\n..##.##...\n##......##\n\"\"\" }\n"
     "}\n"
     "layer mario1 {\n"
     "  use mario\n  color rgb(255, 50, 30)\n  x = (t * 6) % 32\n  y = 2\n  scale = 1\n  frame = (t * 6) % 3\n  visible = 1\n"
     "}\n"},
    {"plasma", "Плазма",
     "effect \"plasma\"\n"
     "sprite fullscreen { bitmap \"\"\"\n################################\n################################\n################################\n################################\n################################\n################################\n################################\n################################\n################################\n################################\n################################\n################################\n################################\n################################\n################################\n################################\n\"\"\" }\n"
     "layer plasma1 {\n"
     "  use fullscreen\n  color hsv(sin(nx * 8 + t) * 30 + cos(ny * 6 + t * 0.7) * 30 + t * 20, 1, 0.5 + 0.5 * sin(nx * 5 + ny * 4 + t))\n"
     "  x = 0\n  y = 0\n  scale = 1\n  visible = 1\n"
     "}\n"},
    {"scrolling-text", "Бегущая строка",
     R"RAW(effect "scrolling_text"
text msg "MYLAMP "
layer scroller {
  use msg
  color rgb(255, 200, 50)
  x = (t * 6) % 32
  y = 5
  scale = 1
  visible = 1
}
)RAW"},
    {"snake", "Змейка",
     "effect \"snake\"\n"
     "sprite dot { bitmap \"\"\"\n#\n\"\"\" }\n"
     "for j = 0; j < 10; j = j + 1 {\n"
     "  layer seg {\n    use dot\n    color hsv(j * 30, 1, 1)\n    x = sin(t * 2 + j * 0.6) * 12 + 16\n    y = cos(t * 2 + j * 0.6) * 6 + 8\n    scale = 1 + (j % 2)\n    visible = 1\n  }\n"
     "}\n"},
    {"fire-particles", "Огоньки",
     "effect \"fire_particles\"\n"
     "sprite dot { bitmap \"\"\"\n#\n\"\"\" }\n"
     "for j = 0; j < 12; j = j + 1 {\n"
     "  layer p {\n    use dot\n    color hsv(15 + j * 2, 1, 1 - j * 0.08)\n    x = sin(j * 2.7 + t) * 3 + 8\n    y = (t * 3 + j * 2) % 32\n    scale = 1 + (j % 2)\n    blend = add\n    visible = 1\n  }\n"
     "}\n"},
    {"starfield", "Звёздное поле",
     "effect \"starfield\"\n"
     "sprite dot { bitmap \"\"\"\n#\n\"\"\" }\n"
     "for j = 0; j < 20; j = j + 1 {\n"
     "  layer s {\n    use dot\n    color rgb(200 + 55 * sin(t + j), 200 + 55 * cos(t + j * 0.7), 255)\n    x = (j * 7 + 3) % 16\n    y = (t * (3 + j % 4) + j * 11) % 32\n    scale = 1\n    blend = add\n    visible = 1\n  }\n"
     "}\n"},
    {"dna", "Спираль ДНК",
     "effect \"dna\"\n"
     "sprite dot { bitmap \"\"\"\n#\n\"\"\" }\n"
     "for j = 0; j < 16; j = j + 1 {\n"
     "  layer h1 {\n    use dot\n    color rgb(80, 200, 255)\n    x = 8 + sin(t * 2 + j * 0.4) * 6\n    y = j * 2\n    scale = 1\n    visible = 1\n  }\n"
     "  layer h2 {\n    use dot\n    color rgb(255, 80, 120)\n    x = 8 + sin(t * 2 + j * 0.4 + 3.1415) * 6\n    y = j * 2\n    scale = 1\n    visible = 1\n  }\n"
     "}\n"},
  };

  for (const DemoEntry& demo : demos) {
    lamp::live::PresetModel preset;
    preset.id = demo.id;
    preset.name = demo.name;
    preset.source = demo.source;
    preset.createdAt = "2026-06-02T00:00:00Z";
    preset.updatedAt = "2026-06-02T00:00:00Z";

    if (g_presetRepository.save(preset)) {
      Serial.print("factory: seeded preset ");
      Serial.println(demo.id);
    } else {
      Serial.print("factory: failed to seed preset ");
      Serial.println(demo.id);
    }
  }
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

lamp::web::TimeStatusSnapshot buildTimeStatusSnapshot();  // forward decl

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
  snapshot.syncStatus = buildTimeStatusSnapshot().syncStatus;
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
  snapshot.fps = g_frameCount > 0 && (millis() - g_lastFpsReportMs) > 0
                     ? g_frameCount * 1000UL / (millis() - g_lastFpsReportMs)
                     : 0;
  snapshot.loopUs = g_lastLoopUs;

  // Compute min/max/avg frame time over the ring buffer window.
  if (g_frameTimeFilled > 0) {
    uint32_t frameMin = UINT32_MAX;
    uint32_t frameMax = 0;
    uint64_t frameSum = 0;
    for (size_t i = 0; i < g_frameTimeFilled; ++i) {
      const uint32_t ft = g_frameTimesUs[i];
      if (ft < frameMin) frameMin = ft;
      if (ft > frameMax) frameMax = ft;
      frameSum += ft;
    }
    snapshot.frameTimeMinUs = frameMin;
    snapshot.frameTimeMaxUs = frameMax;
    snapshot.frameTimeAvgUs = static_cast<uint32_t>(frameSum / g_frameTimeFilled);
  }

  return snapshot;
}

lamp::web::TimeStatusSnapshot buildTimeStatusSnapshot() {
  lamp::web::TimeStatusSnapshot snapshot;
  snapshot.currentTime = g_runtimeTimeState.hasValidTime ? g_timeSource.formattedTime() : "";
  snapshot.timezone = g_settings.clock.timezone;

  // Map ArduinoNtpTimeSource internal status to API string.
  switch (g_timeSource.lastSyncStatus()) {
    case lamp::time::NtpSyncStatus::kSynced:
      snapshot.syncStatus = "ntp_synced";
      break;
    case lamp::time::NtpSyncStatus::kPending:
      snapshot.syncStatus = "ntp_pending";
      break;
    case lamp::time::NtpSyncStatus::kFailed:
      snapshot.syncStatus = "ntp_failed";
      break;
    case lamp::time::NtpSyncStatus::kDisabled:
      snapshot.syncStatus = "ntp_disabled";
      break;
    case lamp::time::NtpSyncStatus::kCached:
      snapshot.syncStatus = "cached";
      break;
  }

  snapshot.ntpServer = lamp::config::kNtpPrimaryServer;
  snapshot.epoch = static_cast<long>(g_timeSource.lastEpoch());
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
  if (!lamp::settings::saveSettingsIfReady(g_settings, g_settingsBackend)) {
    Serial.println("settings: save skipped because NVS backend is unavailable");
  }
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

  FastLED.addLeds<WS2812B, lamp::config::kLedDataPin, GRB>(g_leds, lamp::config::kPixelCount);
  FastLED.setBrightness(lamp::config::kDefaultBrightness);
  FastLED.clear();
  FastLED.show();

  initializeFileSystem();
  if (!g_settingsBackend.begin()) {
    Serial.println("settings: failed to initialize NVS backend, using defaults");
  } else {
    lamp::settings::loadSettingsIfReady(g_settingsBackend, g_settings);
  }
  g_webServer.setSettingsCallbacks(getCurrentSettings, saveAndApplySettings);
  g_webServer.setUpdateCallbacks(checkForFirmwareUpdates, installFirmwareUpdate);
  g_webServer.setTimeStatusCallback(buildTimeStatusSnapshot);
  g_webServer.setPresetServices(&g_presetRepository, &g_liveProgramService);
  g_webServer.setPlaylistServices(&g_playlistRepository, &g_presetRepository, &g_playlistScheduler,
                                  &g_liveProgramService);
  const lamp::network::WiFiStartupResult wifiResult =
      g_wifiManager.startup(g_settings.network, g_wifiAdapter);
  refreshRuntimeState(wifiResult);
  g_effectRegistry.add(g_bootEffect);
  refreshSensorState();
  g_webServer.setStatusSnapshot(buildStatusSnapshot());
  g_webServer.begin();

  // Auto-start fireplace effect at boot.
  {
    std::vector<lamp::live::Diagnostic> diag;
    g_liveProgramService.runTemporary(kFireplaceSource, diag);
  }

  renderFrame(0);
#if APP_IS_DEV
  printBootBanner();
#endif
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
    g_webServer.setStatusSnapshot(buildStatusSnapshot());
#if APP_IS_DEV
    Serial.print("heartbeat uptime_ms=");
    Serial.println(now);
    Serial.print("network status: ");
    Serial.println(g_networkState.statusLine.c_str());
    Serial.print("clock state: ");
    Serial.println(g_timeState.statusLine.c_str());
    Serial.print("sensor state: ");
    Serial.println(g_sensorState.statusLine.c_str());
#endif
  }
  const unsigned long loopStart = micros();
  renderFrame(now);
  g_lastLoopUs = micros() - loopStart;
  // Cap frame rate for simple effects to avoid burning CPU.
  // Complex effects that already exceed the target are unaffected.
  if (g_lastLoopUs < lamp::config::kTargetFrameTimeUs) {
    delayMicroseconds(lamp::config::kTargetFrameTimeUs - g_lastLoopUs);
    g_lastLoopUs = micros() - loopStart;  // re-measure to include delay
  }

  // Store frame time in ring buffer for min/max/avg diagnostics.
  g_frameTimesUs[g_frameTimeIndex] = static_cast<uint32_t>(g_lastLoopUs);
  g_frameTimeIndex = (g_frameTimeIndex + 1) % kFrameTimeWindow;
  if (g_frameTimeFilled < kFrameTimeWindow) {
    ++g_frameTimeFilled;
  }

  ++g_frameCount;
  if (now - g_lastFpsReportMs >= 5000UL) {
    g_lastFpsReportMs = now;
    g_frameCount = 0;
    // Reset frame timing window aligned with FPS window.
    g_frameTimeIndex = 0;
    g_frameTimeFilled = 0;
  }
}
