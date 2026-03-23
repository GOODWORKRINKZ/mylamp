#include "storage/LittleFsFileStore.h"

#include <FS.h>

namespace lamp::storage {

namespace {

std::string parentDirectoryForPath(const std::string& path) {
  const std::string::size_type lastSlash = path.find_last_of('/');
  if (lastSlash == std::string::npos || lastSlash == 0) {
    return "/";
  }

  return path.substr(0, lastSlash);
}

}  // namespace

LittleFsFileStore::LittleFsFileStore(fs::FS& filesystem) : filesystem_(filesystem) {}

bool LittleFsFileStore::writeText(const std::string& path, const std::string& content) {
  if (!ensureParentDirectory(path)) {
    return false;
  }

  File file = filesystem_.open(path.c_str(), FILE_WRITE);
  if (!file) {
    return false;
  }

  const size_t bytesWritten = file.print(content.c_str());
  file.close();
  return bytesWritten == content.length();
}

bool LittleFsFileStore::readText(const std::string& path, std::string& content) const {
  File file = filesystem_.open(path.c_str(), FILE_READ);
  if (!file || file.isDirectory()) {
    return false;
  }

  content.clear();
  while (file.available()) {
    content.push_back(static_cast<char>(file.read()));
  }
  file.close();
  return true;
}

bool LittleFsFileStore::remove(const std::string& path) {
  return filesystem_.remove(path.c_str());
}

std::vector<std::string> LittleFsFileStore::list(const std::string& prefix) const {
  std::vector<std::string> paths;
  File directory = filesystem_.open(prefix.c_str(), FILE_READ);
  if (!directory || !directory.isDirectory()) {
    return paths;
  }

  File entry = directory.openNextFile();
  while (entry) {
    if (!entry.isDirectory()) {
      paths.push_back(std::string(entry.name()));
    }
    entry = directory.openNextFile();
  }

  return paths;
}

bool LittleFsFileStore::ensureParentDirectory(const std::string& path) {
  const std::string parentDirectory = parentDirectoryForPath(path);
  if (parentDirectory == "/") {
    return true;
  }

  if (filesystem_.exists(parentDirectory.c_str())) {
    return true;
  }

  return filesystem_.mkdir(parentDirectory.c_str());
}

}  // namespace lamp::storage