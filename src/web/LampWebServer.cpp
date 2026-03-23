#include "web/LampWebServer.h"

namespace lamp::web {

LampWebServer::LampWebServer() : server_(80) {}

void LampWebServer::begin() {
  registerRoutes();
  server_.begin();
}

void LampWebServer::loop() {
  server_.handleClient();
}

void LampWebServer::setStatusSnapshot(StatusSnapshot snapshot) {
  snapshot_ = snapshot;
}

void LampWebServer::registerRoutes() {
  server_.on("/", [this]() { handleRoot(); });
  server_.on("/api/status", [this]() { handleStatus(); });
}

void LampWebServer::handleRoot() {
  server_.send(200, "text/plain", "mylamp web server");
}

void LampWebServer::handleStatus() {
  server_.send(200, "application/json", buildStatusJson(snapshot_).c_str());
}

}  // namespace lamp::web
