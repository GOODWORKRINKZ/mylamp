#include <unity.h>

#include <string>
#include <vector>

#include "live/PlaylistRepository.h"
#include "live/PresetRepository.h"
#include "live/runtime/LiveProgramService.h"
#include "live/runtime/PlaylistScheduler.h"
#include "storage/IFileStore.h"
#include "web/PlaylistApi.h"

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

std::string makeSource() {
  return "effect \"dot\"\n"
         "sprite dot {\n"
         "  bitmap \"\"\"\n"
         "  #\n"
         "  \"\"\"\n"
         "}\n"
         "layer dot1 {\n"
         "  use dot\n"
         "  color rgb(255, 40, 10)\n"
         "  x = 0\n"
         "  y = 0\n"
         "  scale = 1\n"
         "  visible = 1\n"
         "}\n";
}

lamp::live::PresetModel makePreset(const char* id) {
  lamp::live::PresetModel preset;
  preset.id = id;
  preset.name = id;
  preset.source = makeSource();
  preset.createdAt = "2026-03-23T18:30:00Z";
  preset.updatedAt = "2026-03-23T18:45:00Z";
  return preset;
}

std::string makePlaylistBody(const char* name, bool repeat) {
  return std::string("{") +
         "\"name\":\"" + name + "\"," +
         "\"repeat\":" + (repeat ? "true" : "false") + "," +
         "\"entries\":[{" +
         "\"presetId\":\"warm\"," +
         "\"durationSec\":5," +
         "\"enabled\":true},{" +
         "\"presetId\":\"cool\"," +
         "\"durationSec\":10," +
         "\"enabled\":true}]}";
}

void test_playlist_api_helpers_create_update_delete_start_and_stop() {
  MemoryFileStore fileStore;
  lamp::live::PlaylistRepository playlistRepository(fileStore);
  lamp::live::PresetRepository presetRepository(fileStore);
  lamp::live::runtime::LiveProgramService runtimeService;
  lamp::live::runtime::PlaylistScheduler scheduler;
  std::vector<lamp::live::Diagnostic> diagnostics;
  TEST_ASSERT_TRUE(presetRepository.save(makePreset("warm")));
  TEST_ASSERT_TRUE(presetRepository.save(makePreset("cool")));

  const lamp::web::PlaylistApiResponse createResponse =
      lamp::web::handlePutPlaylistRequest(playlistRepository, "evening",
                                          makePlaylistBody("Evening Loop", true));
  TEST_ASSERT_EQUAL_INT(200, createResponse.statusCode);
  TEST_ASSERT_NOT_EQUAL(-1,
                        static_cast<int>(createResponse.body.find("\"id\":\"evening\"")));

  const lamp::web::PlaylistApiResponse updateResponse =
      lamp::web::handlePutPlaylistRequest(playlistRepository, "evening",
                                          makePlaylistBody("Late Evening", false));
  TEST_ASSERT_EQUAL_INT(200, updateResponse.statusCode);
  TEST_ASSERT_NOT_EQUAL(-1,
                        static_cast<int>(updateResponse.body.find("\"name\":\"Late Evening\"")));

  const lamp::web::PlaylistApiResponse startResponse = lamp::web::handleStartPlaylistRequest(
      playlistRepository, presetRepository, scheduler, runtimeService, "evening", diagnostics);
  TEST_ASSERT_EQUAL_INT(200, startResponse.statusCode);
  TEST_ASSERT_NOT_EQUAL(-1,
                        static_cast<int>(startResponse.body.find("\"activePlaylistId\":\"evening\"")));
  TEST_ASSERT_TRUE(scheduler.state().active);

  const lamp::web::PlaylistApiResponse stopResponse =
      lamp::web::handleStopPlaylistRequest(scheduler, runtimeService);
  TEST_ASSERT_EQUAL_INT(200, stopResponse.statusCode);
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(stopResponse.body.find("\"ok\":true")));
  TEST_ASSERT_FALSE(scheduler.state().active);

  const lamp::web::PlaylistApiResponse deleteResponse =
      lamp::web::handleDeletePlaylistRequest(playlistRepository, "evening");
  TEST_ASSERT_EQUAL_INT(200, deleteResponse.statusCode);
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_playlist_api_helpers_create_update_delete_start_and_stop);
  return UNITY_END();
}