#pragma once

#include <string>

namespace lamp::web {

struct StatusSnapshot {
  std::string version;
  std::string channel;
  std::string board;
  std::string networkMode;
  std::string networkStatus;
  std::string clockStatus;
  std::string activeEffect;
};

std::string buildStatusJson(const StatusSnapshot& snapshot);

}  // namespace lamp::web
