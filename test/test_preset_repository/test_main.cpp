#include <unity.h>

#include <string>
#include <vector>

#include "live/PresetRepository.h"
#include "storage/IFileStore.h"

namespace {

class MemoryFileStore : public lamp::storage::IFileStore {
 public:
  bool isReady() const override {
    return true;
  }

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

class UnreadyFileStore : public lamp::storage::IFileStore {
 public:
  bool isReady() const override {
    return false;
  }

  bool writeText(const std::string&, const std::string&) override {
    ++writeCalls;
    return false;
  }

  bool readText(const std::string&, std::string&) const override {
    ++readCalls;
    return false;
  }

  bool remove(const std::string&) override {
    ++removeCalls;
    return false;
  }

  std::vector<std::string> list(const std::string&) const override {
    ++listCalls;
    return {};
  }

  mutable int readCalls = 0;
  mutable int listCalls = 0;
  int writeCalls = 0;
  int removeCalls = 0;
};

lamp::live::PresetModel makePreset(const char* id, const char* name, float brightnessCap) {
  lamp::live::PresetModel preset;
  preset.id = id;
  preset.name = name;
  preset.source = "effect sample";
  preset.createdAt = "2026-03-23T18:30:00Z";
  preset.updatedAt = "2026-03-23T18:45:00Z";
  preset.tags = {"demo"};
  preset.options.brightnessCap = brightnessCap;
  return preset;
}

void test_preset_repository_saves_loads_lists_and_removes_presets() {
  MemoryFileStore fileStore;
  lamp::live::PresetRepository repository(fileStore);

  TEST_ASSERT_TRUE(repository.save(makePreset("warm_waves", "Warm Waves", 0.35f)));
  TEST_ASSERT_TRUE(repository.save(makePreset("soft_clock", "Soft Clock", 0.25f)));

  lamp::live::PresetModel loaded;
  TEST_ASSERT_TRUE(repository.load("warm_waves", loaded));
  TEST_ASSERT_EQUAL_STRING("Warm Waves", loaded.name.c_str());

  const std::vector<lamp::live::PresetModel> presets = repository.list();
  TEST_ASSERT_EQUAL_UINT32(2U, static_cast<uint32_t>(presets.size()));
  TEST_ASSERT_EQUAL_STRING("warm_waves", presets[0].id.c_str());
  TEST_ASSERT_EQUAL_STRING("soft_clock", presets[1].id.c_str());

  TEST_ASSERT_TRUE(repository.remove("warm_waves"));
  TEST_ASSERT_FALSE(repository.load("warm_waves", loaded));
}

void test_preset_repository_overwrites_existing_preset_by_id() {
  MemoryFileStore fileStore;
  lamp::live::PresetRepository repository(fileStore);

  TEST_ASSERT_TRUE(repository.save(makePreset("warm_waves", "Warm Waves", 0.35f)));
  TEST_ASSERT_TRUE(repository.save(makePreset("warm_waves", "Warmer Waves", 0.45f)));

  lamp::live::PresetModel loaded;
  TEST_ASSERT_TRUE(repository.load("warm_waves", loaded));
  TEST_ASSERT_EQUAL_STRING("Warmer Waves", loaded.name.c_str());
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.45f, loaded.options.brightnessCap);
}

void test_preset_repository_skips_storage_operations_when_store_is_not_ready() {
  UnreadyFileStore fileStore;
  lamp::live::PresetRepository repository(fileStore);

  lamp::live::PresetModel loaded;
  TEST_ASSERT_FALSE(repository.isReady());
  TEST_ASSERT_FALSE(repository.save(makePreset("warm_waves", "Warm Waves", 0.35f)));
  TEST_ASSERT_FALSE(repository.load("warm_waves", loaded));
  TEST_ASSERT_FALSE(repository.remove("warm_waves"));
  TEST_ASSERT_EQUAL_UINT32(0U, static_cast<uint32_t>(repository.list().size()));

  TEST_ASSERT_EQUAL_INT(0, fileStore.writeCalls);
  TEST_ASSERT_EQUAL_INT(0, fileStore.readCalls);
  TEST_ASSERT_EQUAL_INT(0, fileStore.removeCalls);
  TEST_ASSERT_EQUAL_INT(0, fileStore.listCalls);
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_preset_repository_saves_loads_lists_and_removes_presets);
  RUN_TEST(test_preset_repository_overwrites_existing_preset_by_id);
  RUN_TEST(test_preset_repository_skips_storage_operations_when_store_is_not_ready);
  return UNITY_END();
}