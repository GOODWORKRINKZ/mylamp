#include <unity.h>

#include <string>
#include <vector>

#include "live/PresetRepository.h"
#include "live/PlaylistModel.h"
#include "live/runtime/LiveProgramService.h"
#include "live/runtime/PlaylistScheduler.h"
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

std::string makeSourceForColor(uint32_t red, uint32_t green, uint32_t blue) {
  return std::string("effect \"dot\"\n") +
         "sprite dot {\n"
         "  bitmap \"\"\"\n"
         "  #\n"
         "  \"\"\"\n"
         "}\n"
         "layer dot1 {\n"
         "  use dot\n"
         "  color rgb(" + std::to_string(red) + ", " + std::to_string(green) + ", " + std::to_string(blue) + ")\n"
         "  x = 0\n"
         "  y = 0\n"
         "  scale = 1\n"
         "  visible = 1\n"
         "}\n";
}

lamp::live::PresetModel makePreset(const char* id, const std::string& source) {
  lamp::live::PresetModel preset;
  preset.id = id;
  preset.name = id;
  preset.source = source;
  preset.createdAt = "2026-03-23T18:30:00Z";
  preset.updatedAt = "2026-03-23T18:45:00Z";
  return preset;
}

lamp::live::PlaylistModel makePlaylist(bool repeat) {
  lamp::live::PlaylistModel playlist;
  playlist.id = "evening";
  playlist.name = "Evening";
  playlist.repeat = repeat;
  playlist.entries.push_back({"warm", 5U, true});
  playlist.entries.push_back({"cool", 10U, true});
  return playlist;
}

void seedPresets(lamp::live::PresetRepository& repository) {
  TEST_ASSERT_TRUE(repository.save(makePreset("warm", makeSourceForColor(255U, 40U, 10U))));
  TEST_ASSERT_TRUE(repository.save(makePreset("cool", makeSourceForColor(10U, 40U, 255U))));
}

void test_playlist_scheduler_starts_playlist_and_activates_first_preset() {
  MemoryFileStore fileStore;
  lamp::live::PresetRepository repository(fileStore);
  seedPresets(repository);
  lamp::live::runtime::LiveProgramService runtimeService;
  lamp::live::runtime::PlaylistScheduler scheduler;
  std::vector<lamp::live::Diagnostic> diagnostics;

  TEST_ASSERT_TRUE(scheduler.start(makePlaylist(false), repository, runtimeService, diagnostics));
  TEST_ASSERT_EQUAL_UINT32(0U, static_cast<uint32_t>(diagnostics.size()));
  TEST_ASSERT_TRUE(scheduler.state().active);
  TEST_ASSERT_TRUE(scheduler.state().autoplayActive);
  TEST_ASSERT_EQUAL_STRING("evening", scheduler.state().activePlaylistId.c_str());
  TEST_ASSERT_EQUAL_STRING("warm", runtimeService.state().activePresetId.c_str());
}

void test_playlist_scheduler_advances_by_duration() {
  MemoryFileStore fileStore;
  lamp::live::PresetRepository repository(fileStore);
  seedPresets(repository);
  lamp::live::runtime::LiveProgramService runtimeService;
  lamp::live::runtime::PlaylistScheduler scheduler;
  std::vector<lamp::live::Diagnostic> diagnostics;
  TEST_ASSERT_TRUE(scheduler.start(makePlaylist(false), repository, runtimeService, diagnostics));

  TEST_ASSERT_TRUE(scheduler.advance(5000U, repository, runtimeService, diagnostics));

  TEST_ASSERT_EQUAL_UINT16(1U, scheduler.state().activeEntryIndex);
  TEST_ASSERT_EQUAL_STRING("cool", runtimeService.state().activePresetId.c_str());
}

void test_playlist_scheduler_repeats_from_start_when_enabled() {
  MemoryFileStore fileStore;
  lamp::live::PresetRepository repository(fileStore);
  seedPresets(repository);
  lamp::live::runtime::LiveProgramService runtimeService;
  lamp::live::runtime::PlaylistScheduler scheduler;
  std::vector<lamp::live::Diagnostic> diagnostics;
  TEST_ASSERT_TRUE(scheduler.start(makePlaylist(true), repository, runtimeService, diagnostics));

  TEST_ASSERT_TRUE(scheduler.advance(5000U, repository, runtimeService, diagnostics));
  TEST_ASSERT_TRUE(scheduler.advance(10000U, repository, runtimeService, diagnostics));

  TEST_ASSERT_TRUE(scheduler.state().active);
  TEST_ASSERT_EQUAL_UINT16(0U, scheduler.state().activeEntryIndex);
  TEST_ASSERT_EQUAL_STRING("warm", runtimeService.state().activePresetId.c_str());
}

void test_playlist_scheduler_stops_when_manual_run_disables_autoplay() {
  MemoryFileStore fileStore;
  lamp::live::PresetRepository repository(fileStore);
  seedPresets(repository);
  lamp::live::runtime::LiveProgramService runtimeService;
  lamp::live::runtime::PlaylistScheduler scheduler;
  std::vector<lamp::live::Diagnostic> diagnostics;
  TEST_ASSERT_TRUE(scheduler.start(makePlaylist(true), repository, runtimeService, diagnostics));

  TEST_ASSERT_TRUE(runtimeService.runTemporary(makeSourceForColor(1U, 2U, 3U), diagnostics));
  scheduler.syncWithRuntime(runtimeService);

  TEST_ASSERT_FALSE(scheduler.state().active);
  TEST_ASSERT_FALSE(scheduler.state().autoplayActive);
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_playlist_scheduler_starts_playlist_and_activates_first_preset);
  RUN_TEST(test_playlist_scheduler_advances_by_duration);
  RUN_TEST(test_playlist_scheduler_repeats_from_start_when_enabled);
  RUN_TEST(test_playlist_scheduler_stops_when_manual_run_disables_autoplay);
  return UNITY_END();
}