#pragma once

#include <functional>

#include <WebServer.h>

#include "live/Diagnostic.h"
#include "live/LiveRequestJson.h"
#include "settings/AppSettings.h"
#include "web/EmbeddedAsset.h"
#include "web/NetworkSettingsJson.h"
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

 private:
  void registerRoutes();
  void handleRoot();
  void handleScript();
  void handleStyles();
  void handleStatus();
  void handleGetNetworkSettings();
  void handleUpdateNetworkSettings();
  void handleLiveValidate();
  void handleLiveRun();
  void sendEmbeddedAsset(const EmbeddedAsset& asset);

  WebServer server_;
  StatusSnapshot snapshot_;
  SettingsGetter getSettings_;
  SettingsSaver saveSettings_;
};

}  // namespace lamp::web
