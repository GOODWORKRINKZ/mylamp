#include <unity.h>

#include <string>

#include "live/PresetJson.h"

namespace {

void test_preset_json_round_trip_preserves_fields() {
  lamp::live::PresetModel preset;
  preset.id = "warm_waves";
  preset.name = "Warm Waves";
  preset.source = "effect warm_waves";
  preset.createdAt = "2026-03-23T18:30:00Z";
  preset.updatedAt = "2026-03-23T18:45:00Z";
  preset.tags = {"warm", "ambient"};
  preset.options.brightnessCap = 0.35f;

  const std::string json = lamp::live::buildPresetJson(preset);
  lamp::live::PresetModel parsed;

  TEST_ASSERT_TRUE(lamp::live::parsePresetJson(json, parsed));
  TEST_ASSERT_EQUAL_STRING("warm_waves", parsed.id.c_str());
  TEST_ASSERT_EQUAL_STRING("Warm Waves", parsed.name.c_str());
  TEST_ASSERT_EQUAL_STRING("effect warm_waves", parsed.source.c_str());
  TEST_ASSERT_EQUAL_STRING("2026-03-23T18:30:00Z", parsed.createdAt.c_str());
  TEST_ASSERT_EQUAL_STRING("2026-03-23T18:45:00Z", parsed.updatedAt.c_str());
  TEST_ASSERT_EQUAL_UINT32(2U, static_cast<uint32_t>(parsed.tags.size()));
  TEST_ASSERT_EQUAL_STRING("warm", parsed.tags[0].c_str());
  TEST_ASSERT_EQUAL_STRING("ambient", parsed.tags[1].c_str());
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.35f, parsed.options.brightnessCap);
}

void test_preset_json_rejects_missing_required_fields() {
  lamp::live::PresetModel parsed;

  TEST_ASSERT_FALSE(lamp::live::parsePresetJson(
      "{\"name\":\"Warm Waves\",\"source\":\"effect warm_waves\"}", parsed));
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_preset_json_round_trip_preserves_fields);
  RUN_TEST(test_preset_json_rejects_missing_required_fields);
  return UNITY_END();
}