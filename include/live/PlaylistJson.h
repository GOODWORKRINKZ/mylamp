#pragma once

#include <string>

#include "live/PlaylistModel.h"

namespace lamp::live {

std::string buildPlaylistJson(const PlaylistModel& playlist);
bool parsePlaylistJson(const std::string& json, PlaylistModel& playlist);

}  // namespace lamp::live