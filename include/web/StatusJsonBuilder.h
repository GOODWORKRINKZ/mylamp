#pragma once

#include <string>

namespace lamp::web {

struct StatusSnapshot {
  std::string version;
  std::string channel;
  std::string board;
  std::string hardwareType;
  std::string updateChannel;
  std::string updateState;
  std::string availableVersion;
  std::string updateError;
  std::string networkMode;
  std::string networkStatus;
  std::string clockStatus;
  std::string syncStatus;
  std::string currentTime;
  std::string sensorStatus;
  bool sensorAvailable = false;
  float temperatureC = 0.0f;
  float humidityPercent = 0.0f;
  std::string activeEffect;
  std::string activePresetId;
  std::string activePresetName;
  bool autoplayEnabled = false;
  std::string activePlaylistId;
  std::string liveErrorSummary;
  uint32_t fps = 0;
  uint32_t loopUs = 0;
};

std::string buildStatusJson(const StatusSnapshot& snapshot);

}  // namespace lamp::web
