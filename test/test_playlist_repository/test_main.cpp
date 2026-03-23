#include <unity.h>

#include <string>
#include <vector>

#include "live/PlaylistRepository.h"
#include "storage/IFileStore.h"

namespace {

class MemoryFileStore : public lamp::storage::IFileStore {
 public:
  bool writeText(const std::string& path, const std::string& content) override {
    for (Entry& entry : entries_) {
      if (entry.path == path) {
        entry.content = content;
        return true;
      }
    }

    entries_.push_back({path, content});
    return true;
  }

  bool readText(const std::string& path, std::string& content) const override {
    for (const Entry& entry : entries_) {
      if (entry.path == path) {
        content = entry.content;
        return true;
      }
    }

    return false;
  }

  bool remove(const std::string& path) override {
    for (std::vector<Entry>::iterator it = entries_.begin(); it != entries_.end(); ++it) {
      if (it->path == path) {
        entries_.erase(it);
        return true;
      }
    }

    return false;
  }

  std::vector<std::string> list(const std::string& prefix) const override {
    std::vector<std::string> paths;
    for (const Entry& entry : entries_) {
      if (entry.path.rfind(prefix, 0) == 0) {
        paths.push_back(entry.path);
      }
    }

    return paths;
  }

 private:
  struct Entry {
    std::string path;
    std::string content;
  };

  std::vector<Entry> entries_;
};

lamp::live::PlaylistModel makePlaylist(const char* id, const char* name, bool repeat,
                                       uint32_t firstDurationSec) {
  lamp::live::PlaylistModel playlist;
  playlist.id = id;
  playlist.name = name;
  playlist.repeat = repeat;
  playlist.entries.push_back({"warm_waves", firstDurationSec, true});
  playlist.entries.push_back({"soft_clock", 60U, true});
  return playlist;
}

void test_playlist_repository_saves_loads_lists_and_removes_playlists() {
  MemoryFileStore fileStore;
  lamp::live::PlaylistRepository repository(fileStore);

  TEST_ASSERT_TRUE(repository.save(makePlaylist("evening", "Evening Loop", true, 90U)));
  TEST_ASSERT_TRUE(repository.save(makePlaylist("night", "Night Loop", false, 120U)));

  lamp::live::PlaylistModel loaded;
  TEST_ASSERT_TRUE(repository.load("evening", loaded));
  TEST_ASSERT_TRUE(loaded.repeat);
  TEST_ASSERT_EQUAL_UINT32(2U, static_cast<uint32_t>(loaded.entries.size()));

  const std::vector<lamp::live::PlaylistModel> playlists = repository.list();
  TEST_ASSERT_EQUAL_UINT32(2U, static_cast<uint32_t>(playlists.size()));
  TEST_ASSERT_EQUAL_STRING("evening", playlists[0].id.c_str());
  TEST_ASSERT_EQUAL_STRING("night", playlists[1].id.c_str());

  TEST_ASSERT_TRUE(repository.remove("evening"));
  TEST_ASSERT_FALSE(repository.load("evening", loaded));
}

void test_playlist_repository_overwrites_existing_playlist_by_id() {
  MemoryFileStore fileStore;
  lamp::live::PlaylistRepository repository(fileStore);

  TEST_ASSERT_TRUE(repository.save(makePlaylist("evening", "Evening Loop", true, 90U)));
  TEST_ASSERT_TRUE(repository.save(makePlaylist("evening", "Late Evening", false, 45U)));

  lamp::live::PlaylistModel loaded;
  TEST_ASSERT_TRUE(repository.load("evening", loaded));
  TEST_ASSERT_EQUAL_STRING("Late Evening", loaded.name.c_str());
  TEST_ASSERT_FALSE(loaded.repeat);
  TEST_ASSERT_EQUAL_UINT32(45U, loaded.entries[0].durationSec);
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_playlist_repository_saves_loads_lists_and_removes_playlists);
  RUN_TEST(test_playlist_repository_overwrites_existing_playlist_by_id);
  return UNITY_END();
}