#pragma once

#include <string>
#include <vector>

#include "live/PlaylistModel.h"
#include "storage/IFileStore.h"

namespace lamp::live {

class PlaylistRepository {
 public:
  explicit PlaylistRepository(storage::IFileStore& fileStore);

  bool isReady() const;
  bool save(const PlaylistModel& playlist);
  bool load(const std::string& id, PlaylistModel& playlist) const;
  std::vector<PlaylistModel> list() const;
  bool remove(const std::string& id);

 private:
  storage::IFileStore& fileStore_;
};

}  // namespace lamp::live