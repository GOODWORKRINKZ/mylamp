#pragma once

#include <functional>

#include <WebServer.h>

#include "live/Diagnostic.h"
#include "live/LiveRequestJson.h"
#include "live/PlaylistRepository.h"
#include "live/PresetRepository.h"
#include "live/runtime/LiveProgramService.h"
#include "live/runtime/PlaylistScheduler.h"
#include "settings/AppSettings.h"
#include "web/EmbeddedAsset.h"
#include "web/NetworkSettingsJson.h"
#include "web/PlaylistApi.h"
#include "web/PresetApi.h"
#include "web/StatusJsonBuilder.h"

namespace lamp::web {

class LampWebServer {
 public:
  using SettingsGetter = std::function<settings::AppSettings()>;
  using SettingsSaver = std::function<void(const settings::AppSettings&)>;

  LampWebServer();

  void begin();
  void loop();
  void setStatusSnapshot(StatusSnapshot snapshot);
  void setSettingsCallbacks(SettingsGetter getter, SettingsSaver saver);
  void setPresetServices(live::PresetRepository* repository,
                         live::runtime::LiveProgramService* runtimeService);
  void setPlaylistServices(live::PlaylistRepository* playlistRepository,
                           live::PresetRepository* presetRepository,
                           live::runtime::PlaylistScheduler* scheduler,
                           live::runtime::LiveProgramService* runtimeService);

 private:
  void registerRoutes();
  void handleNotFound();
  void handleRoot();
  void handleFavicon();
  void handleScript();
  void handleStyles();
  void handleStatus();
  void handleGetNetworkSettings();
  void handleUpdateNetworkSettings();
  void handleLiveValidate();
  void handleLiveRun();
  void handleListPresets();
  void handlePutPreset();
  void handlePresetByPath();
  void handlePlaylistByPath();
  void sendEmbeddedAsset(const EmbeddedAsset& asset);

  WebServer server_;
  StatusSnapshot snapshot_;
  SettingsGetter getSettings_;
  SettingsSaver saveSettings_;
  live::PresetRepository* presetRepository_ = nullptr;
  live::PlaylistRepository* playlistRepository_ = nullptr;
  live::runtime::PlaylistScheduler* playlistScheduler_ = nullptr;
  live::runtime::LiveProgramService* liveProgramService_ = nullptr;
};

}  // namespace lamp::web
