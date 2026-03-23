#pragma once

#include <string>
#include <vector>

#include "live/PresetModel.h"
#include "storage/IFileStore.h"

namespace lamp::live {

class PresetRepository {
 public:
  explicit PresetRepository(storage::IFileStore& fileStore);

  bool isReady() const;
  bool save(const PresetModel& preset);
  bool load(const std::string& id, PresetModel& preset) const;
  std::vector<PresetModel> list() const;
  bool remove(const std::string& id);

 private:
  storage::IFileStore& fileStore_;
};

}  // namespace lamp::live