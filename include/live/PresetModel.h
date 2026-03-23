#pragma once

#include <string>
#include <vector>

namespace lamp::live {

struct PresetOptions {
  float brightnessCap = 1.0f;
};

struct PresetModel {
  std::string id;
  std::string name;
  std::string source;
  std::string createdAt;
  std::string updatedAt;
  std::vector<std::string> tags;
  PresetOptions options;
};

}  // namespace lamp::live