#pragma once

#include <stdint.h>

#include <string>
#include <vector>

namespace lamp::live {

struct PlaylistEntry {
  std::string presetId;
  uint32_t durationSec = 0;
  bool enabled = true;
};

struct PlaylistModel {
  std::string id;
  std::string name;
  bool repeat = false;
  std::vector<PlaylistEntry> entries;
};

}  // namespace lamp::live