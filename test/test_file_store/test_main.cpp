#include <unity.h>

#include <string>
#include <vector>

#include "storage/ContentPaths.h"
#include "storage/IFileStore.h"
#include "storage/StorageBootstrap.h"

namespace {

class MemoryFileStore : public lamp::storage::IFileStore {
 public:
  bool isReady() const override {
    return true;
  }

  bool writeText(const std::string& path, const std::string& content) override {
    for (auto& entry : files_) {
      if (entry.path == path) {
        entry.content = content;
        return true;
      }
    }

    files_.push_back({path, content});
    return true;
  }

  bool readText(const std::string& path, std::string& content) const override {
    for (const auto& entry : files_) {
      if (entry.path == path) {
        content = entry.content;
        return true;
      }
    }

    return false;
  }

  bool remove(const std::string& path) override {
    for (std::vector<Entry>::iterator it = files_.begin(); it != files_.end(); ++it) {
      if (it->path == path) {
        files_.erase(it);
        return true;
      }
    }

    return false;
  }

  std::vector<std::string> list(const std::string& prefix) const override {
    std::vector<std::string> paths;
    for (const auto& entry : files_) {
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

  std::vector<Entry> files_;
};

class FakeFileSystem {
 public:
  bool exists(const char* path) const {
    for (const std::string& directory : directories_) {
      if (directory == path) {
        return true;
      }
    }

    return false;
  }

  bool mkdir(const char* path) {
    mkdirCalls_.push_back(path);
    if (failOnMkdir_) {
      return false;
    }

    directories_.push_back(path);
    return true;
  }

  void addDirectory(const char* path) {
    directories_.push_back(path);
  }

  void setFailOnMkdir(bool failOnMkdir) {
    failOnMkdir_ = failOnMkdir;
  }

  const std::vector<std::string>& mkdirCalls() const {
    return mkdirCalls_;
  }

 private:
  std::vector<std::string> directories_;
  std::vector<std::string> mkdirCalls_;
  bool failOnMkdir_ = false;
};

void test_content_paths_build_expected_locations() {
  TEST_ASSERT_EQUAL_STRING("/presets/warm-waves.json",
                           lamp::storage::presetPath("warm-waves").c_str());
  TEST_ASSERT_EQUAL_STRING("/playlists/evening.json",
                           lamp::storage::playlistPath("evening").c_str());
}

void test_file_store_contract_supports_write_read_list_and_remove() {
  MemoryFileStore fileStore;

  TEST_ASSERT_TRUE(fileStore.writeText("/presets/fire.json", "{\"id\":\"fire\"}"));
  TEST_ASSERT_TRUE(fileStore.writeText("/playlists/evening.json", "{\"id\":\"evening\"}"));

  std::string content;
  TEST_ASSERT_TRUE(fileStore.readText("/presets/fire.json", content));
  TEST_ASSERT_EQUAL_STRING("{\"id\":\"fire\"}", content.c_str());

  const std::vector<std::string> presetPaths = fileStore.list("/presets/");
  TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(presetPaths.size()));
  TEST_ASSERT_EQUAL_STRING("/presets/fire.json", presetPaths[0].c_str());

  TEST_ASSERT_TRUE(fileStore.remove("/presets/fire.json"));
  TEST_ASSERT_FALSE(fileStore.readText("/presets/fire.json", content));
}

void test_storage_bootstrap_creates_missing_content_directories() {
  FakeFileSystem filesystem;

  TEST_ASSERT_TRUE(lamp::storage::ensureContentDirectories(filesystem));
  TEST_ASSERT_EQUAL_UINT32(2U, static_cast<uint32_t>(filesystem.mkdirCalls().size()));
  TEST_ASSERT_EQUAL_STRING("/presets", filesystem.mkdirCalls()[0].c_str());
  TEST_ASSERT_EQUAL_STRING("/playlists", filesystem.mkdirCalls()[1].c_str());
}

void test_storage_bootstrap_is_noop_when_content_directories_already_exist() {
  FakeFileSystem filesystem;
  filesystem.addDirectory("/presets");
  filesystem.addDirectory("/playlists");

  TEST_ASSERT_TRUE(lamp::storage::ensureContentDirectories(filesystem));
  TEST_ASSERT_EQUAL_UINT32(0U, static_cast<uint32_t>(filesystem.mkdirCalls().size()));
}

void test_storage_bootstrap_reports_directory_creation_failures() {
  FakeFileSystem filesystem;
  filesystem.setFailOnMkdir(true);

  TEST_ASSERT_FALSE(lamp::storage::ensureContentDirectories(filesystem));
  TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(filesystem.mkdirCalls().size()));
  TEST_ASSERT_EQUAL_STRING("/presets", filesystem.mkdirCalls()[0].c_str());
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_content_paths_build_expected_locations);
  RUN_TEST(test_file_store_contract_supports_write_read_list_and_remove);
  RUN_TEST(test_storage_bootstrap_creates_missing_content_directories);
  RUN_TEST(test_storage_bootstrap_is_noop_when_content_directories_already_exist);
  RUN_TEST(test_storage_bootstrap_reports_directory_creation_failures);
  return UNITY_END();
}