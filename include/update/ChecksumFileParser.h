#pragma once

#include <string>

namespace lamp::update {

struct ParsedChecksumFile {
  bool valid = false;
  std::string sha256;
  std::string error;
};

ParsedChecksumFile parseChecksumFile(const std::string& payload, const std::string& expectedAssetName);

}  // namespace lamp::update