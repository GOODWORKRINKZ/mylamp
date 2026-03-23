#pragma once

#include <string>

namespace lamp::storage {

static constexpr char kPresetsDirectory[] = "/presets";
static constexpr char kPlaylistsDirectory[] = "/playlists";

inline std::string presetPath(const std::string& presetId) {
  return std::string(kPresetsDirectory) + "/" + presetId + ".json";
}

inline std::string playlistPath(const std::string& playlistId) {
  return std::string(kPlaylistsDirectory) + "/" + playlistId + ".json";
}

}  // namespace lamp::storage