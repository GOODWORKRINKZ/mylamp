#pragma once

#include "storage/ContentPaths.h"

namespace lamp::storage {

template <typename FileSystemLike>
bool ensureContentDirectories(FileSystemLike& filesystem) {
  if (!filesystem.exists(kPresetsDirectory) && !filesystem.mkdir(kPresetsDirectory)) {
    return false;
  }

  if (!filesystem.exists(kPlaylistsDirectory) && !filesystem.mkdir(kPlaylistsDirectory)) {
    return false;
  }

  return true;
}

}  // namespace lamp::storage