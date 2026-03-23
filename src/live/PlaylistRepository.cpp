#include "live/PlaylistRepository.h"

#include "live/PlaylistJson.h"
#include "storage/ContentPaths.h"

namespace lamp::live {

PlaylistRepository::PlaylistRepository(storage::IFileStore& fileStore) : fileStore_(fileStore) {}

bool PlaylistRepository::save(const PlaylistModel& playlist) {
  return fileStore_.writeText(storage::playlistPath(playlist.id), buildPlaylistJson(playlist));
}

bool PlaylistRepository::load(const std::string& id, PlaylistModel& playlist) const {
  std::string json;
  if (!fileStore_.readText(storage::playlistPath(id), json)) {
    return false;
  }

  return parsePlaylistJson(json, playlist);
}

std::vector<PlaylistModel> PlaylistRepository::list() const {
  std::vector<PlaylistModel> playlists;
  const std::vector<std::string> paths = fileStore_.list("/playlists/");
  for (const std::string& path : paths) {
    std::string json;
    if (!fileStore_.readText(path, json)) {
      continue;
    }

    PlaylistModel playlist;
    if (parsePlaylistJson(json, playlist)) {
      playlists.push_back(playlist);
    }
  }

  return playlists;
}

bool PlaylistRepository::remove(const std::string& id) {
  return fileStore_.remove(storage::playlistPath(id));
}

}  // namespace lamp::live