#include "update/ChecksumFileParser.h"

#include <cctype>

namespace lamp::update {

namespace {

bool isHexDigest(const std::string& value) {
  if (value.size() != 64) {
    return false;
  }

  for (char ch : value) {
    if (!std::isxdigit(static_cast<unsigned char>(ch))) {
      return false;
    }
  }

  return true;
}

std::string trim(const std::string& value) {
  const size_t start = value.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) {
    return std::string();
  }

  const size_t end = value.find_last_not_of(" \t\r\n");
  return value.substr(start, end - start + 1);
}

}  // namespace

ParsedChecksumFile parseChecksumFile(const std::string& payload,
                                     const std::string& expectedAssetName) {
  ParsedChecksumFile parsed;

  const std::string line = trim(payload);
  if (line.empty()) {
    parsed.error = "empty-checksum-file";
    return parsed;
  }

  const size_t firstWhitespace = line.find_first_of(" \t");
  if (firstWhitespace == std::string::npos) {
    parsed.error = "invalid-checksum-format";
    return parsed;
  }

  const std::string digest = line.substr(0, firstWhitespace);
  if (!isHexDigest(digest)) {
    parsed.error = "invalid-checksum-format";
    return parsed;
  }

  const std::string fileName = trim(line.substr(firstWhitespace + 1));
  if (fileName != expectedAssetName) {
    parsed.error = "checksum-asset-mismatch";
    return parsed;
  }

  parsed.valid = true;
  parsed.sha256 = digest;
  return parsed;
}

}  // namespace lamp::update