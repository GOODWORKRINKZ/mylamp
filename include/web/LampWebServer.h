#pragma once

#include <WebServer.h>

#include "web/StatusJsonBuilder.h"

namespace lamp::web {

class LampWebServer {
 public:
  LampWebServer();

  void begin();
  void loop();
  void setStatusSnapshot(StatusSnapshot snapshot);

 private:
  void registerRoutes();
  void handleRoot();
  void handleStatus();

  WebServer server_;
  StatusSnapshot snapshot_;
};

}  // namespace lamp::web
