#pragma once

#include <string>

namespace lamp::update {

struct FirmwareReleaseInfo {
  bool available = false;
  std::string channel;
  std::string version;
  std::string assetName;
  std::string assetUrl;
  std::string checksumUrl;
  std::string notes;
  std::string publishedAt;
  std::string error;
};

}  // namespace lamp::update