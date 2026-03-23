#pragma once

#include <FS.h>

#include <string>
#include <vector>

#include "storage/IFileStore.h"

namespace lamp::storage {

class LittleFsFileStore : public IFileStore {
 public:
  explicit LittleFsFileStore(fs::FS& filesystem);

  bool writeText(const std::string& path, const std::string& content) override;
  bool readText(const std::string& path, std::string& content) const override;
  bool remove(const std::string& path) override;
  std::vector<std::string> list(const std::string& prefix) const override;

 private:
  bool ensureParentDirectory(const std::string& path);

  fs::FS& filesystem_;
};

}  // namespace lamp::storage