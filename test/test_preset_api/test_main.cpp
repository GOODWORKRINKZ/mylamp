#include <unity.h>

#include <string>
#include <vector>

#include "live/PresetRepository.h"
#include "live/runtime/LiveProgramService.h"
#include "storage/IFileStore.h"
#include "web/PresetApi.h"

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

std::string makePresetBody(const char* name, const char* source) {
  return std::string("{") +
         "\"name\":\"" + name + "\"," +
         "\"source\":\"" + source + "\"," +
         "\"createdAt\":\"2026-03-23T18:30:00Z\"," +
         "\"updatedAt\":\"2026-03-23T18:45:00Z\"," +
         "\"tags\":[\"warm\"]," +
         "\"options\":{\"brightnessCap\":0.35}}";
}

std::string escapeJsonString(const std::string& value) {
  std::string escaped;
  for (char ch : value) {
    switch (ch) {
      case '\\':
        escaped += "\\\\";
        break;
      case '"':
        escaped += "\\\"";
        break;
      case '\n':
        escaped += "\\n";
        break;
      default:
        escaped.push_back(ch);
        break;
    }
  }
  return escaped;
}

std::string makeValidDslSource() {
  return "effect \"dot\"\n"
         "sprite dot {\n"
         "  bitmap \"\"\"\n"
         "  #\n"
         "  \"\"\"\n"
         "}\n"
         "layer dot1 {\n"
         "  use dot\n"
         "  color rgb(200, 40, 10)\n"
         "  x = 1\n"
         "  y = 2\n"
         "  scale = 1\n"
         "  visible = 1\n"
         "}\n";
}

void test_preset_api_helpers_save_get_and_list_presets() {
  MemoryFileStore fileStore;
  lamp::live::PresetRepository repository(fileStore);

  const lamp::web::PresetApiResponse saveResponse =
      lamp::web::handlePutPresetRequest(repository, "warm_waves",
                      makePresetBody("Warm Waves", escapeJsonString(makeValidDslSource()).c_str()));
  TEST_ASSERT_EQUAL_INT(200, saveResponse.statusCode);
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(saveResponse.body.find("\"id\":\"warm_waves\"")));

  const lamp::web::PresetApiResponse getResponse =
      lamp::web::handleGetPresetRequest(repository, "warm_waves");
  TEST_ASSERT_EQUAL_INT(200, getResponse.statusCode);
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(getResponse.body.find("\"name\":\"Warm Waves\"")));

  const lamp::web::PresetApiResponse listResponse = lamp::web::handleListPresetsRequest(repository);
  TEST_ASSERT_EQUAL_INT(200, listResponse.statusCode);
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(listResponse.body.find("\"items\":")));
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(listResponse.body.find("\"warm_waves\"")));
}

void test_preset_api_helpers_update_activate_and_delete_presets() {
  MemoryFileStore fileStore;
  lamp::live::PresetRepository repository(fileStore);
  lamp::live::runtime::LiveProgramService runtimeService;

  TEST_ASSERT_EQUAL_INT(
      200, lamp::web::handlePutPresetRequest(repository, "warm_waves",
                           makePresetBody("Warm Waves", escapeJsonString(makeValidDslSource()).c_str()))
               .statusCode);

  const lamp::web::PresetApiResponse updateResponse =
      lamp::web::handlePutPresetRequest(repository, "warm_waves",
                        makePresetBody("Warmer Waves", escapeJsonString(makeValidDslSource()).c_str()));
  TEST_ASSERT_EQUAL_INT(200, updateResponse.statusCode);
  TEST_ASSERT_NOT_EQUAL(-1,
                        static_cast<int>(updateResponse.body.find("\"name\":\"Warmer Waves\"")));

  const lamp::web::PresetApiResponse activateResponse =
      lamp::web::handleActivatePresetRequest(repository, runtimeService, "warm_waves");
  TEST_ASSERT_EQUAL_INT(200, activateResponse.statusCode);
  TEST_ASSERT_NOT_EQUAL(-1,
                        static_cast<int>(activateResponse.body.find("\"activePresetId\":\"warm_waves\"")));
  TEST_ASSERT_EQUAL_STRING("warm_waves", runtimeService.state().activePresetId.c_str());

  const lamp::web::PresetApiResponse deleteResponse =
      lamp::web::handleDeletePresetRequest(repository, "warm_waves");
  TEST_ASSERT_EQUAL_INT(200, deleteResponse.statusCode);
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(deleteResponse.body.find("\"ok\":true")));

  const lamp::web::PresetApiResponse missingResponse =
      lamp::web::handleGetPresetRequest(repository, "warm_waves");
  TEST_ASSERT_EQUAL_INT(404, missingResponse.statusCode);
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_preset_api_helpers_save_get_and_list_presets);
  RUN_TEST(test_preset_api_helpers_update_activate_and_delete_presets);
  return UNITY_END();
}