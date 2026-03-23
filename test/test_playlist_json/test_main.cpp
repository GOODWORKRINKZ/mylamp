#include <unity.h>

#include <string>

#include "live/PlaylistJson.h"

namespace {

void test_playlist_json_round_trip_preserves_entries() {
  lamp::live::PlaylistModel playlist;
  playlist.id = "evening";
  playlist.name = "Evening Loop";
  playlist.repeat = true;
  playlist.entries.push_back({"warm_waves", 90U, true});
  playlist.entries.push_back({"soft_clock", 60U, true});
  playlist.entries.push_back({"fire_band", 45U, false});

  const std::string json = lamp::live::buildPlaylistJson(playlist);
  lamp::live::PlaylistModel parsed;

  TEST_ASSERT_TRUE(lamp::live::parsePlaylistJson(json, parsed));
  TEST_ASSERT_EQUAL_STRING("evening", parsed.id.c_str());
  TEST_ASSERT_EQUAL_STRING("Evening Loop", parsed.name.c_str());
  TEST_ASSERT_TRUE(parsed.repeat);
  TEST_ASSERT_EQUAL_UINT32(3U, static_cast<uint32_t>(parsed.entries.size()));
  TEST_ASSERT_EQUAL_STRING("warm_waves", parsed.entries[0].presetId.c_str());
  TEST_ASSERT_EQUAL_UINT32(90U, parsed.entries[0].durationSec);
  TEST_ASSERT_TRUE(parsed.entries[0].enabled);
  TEST_ASSERT_EQUAL_STRING("fire_band", parsed.entries[2].presetId.c_str());
  TEST_ASSERT_EQUAL_UINT32(45U, parsed.entries[2].durationSec);
  TEST_ASSERT_FALSE(parsed.entries[2].enabled);
}

void test_playlist_json_rejects_entry_without_duration() {
  lamp::live::PlaylistModel parsed;

  TEST_ASSERT_FALSE(lamp::live::parsePlaylistJson(
      "{\"id\":\"evening\",\"name\":\"Evening Loop\",\"repeat\":true,\"entries\":[{\"presetId\":\"warm_waves\",\"enabled\":true}]}",
      parsed));
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_playlist_json_round_trip_preserves_entries);
  RUN_TEST(test_playlist_json_rejects_entry_without_duration);
  return UNITY_END();
}