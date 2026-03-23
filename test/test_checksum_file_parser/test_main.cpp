#include <unity.h>

#include <string>

#include "update/ChecksumFileParser.h"

namespace {

void test_parser_reads_sha256_sidecar_for_matching_asset() {
  const std::string payload =
      "4d186321c1a7f0f354b297e8914ab2400d6d5f6d6f8d2c12c6a4eb3a1dcb0a2b  "
      "mylamp-c3-cylinder32x16-v0.2.0-release.bin\n";

  const lamp::update::ParsedChecksumFile parsed = lamp::update::parseChecksumFile(
      payload, "mylamp-c3-cylinder32x16-v0.2.0-release.bin");

  TEST_ASSERT_TRUE(parsed.valid);
  TEST_ASSERT_EQUAL_STRING(
      "4d186321c1a7f0f354b297e8914ab2400d6d5f6d6f8d2c12c6a4eb3a1dcb0a2b",
      parsed.sha256.c_str());
}

void test_parser_rejects_mismatched_asset_name() {
  const std::string payload =
      "4d186321c1a7f0f354b297e8914ab2400d6d5f6d6f8d2c12c6a4eb3a1dcb0a2b  "
      "other.bin\n";

  const lamp::update::ParsedChecksumFile parsed =
      lamp::update::parseChecksumFile(payload, "mylamp-c3-cylinder32x16-v0.2.0-release.bin");

  TEST_ASSERT_FALSE(parsed.valid);
  TEST_ASSERT_EQUAL_STRING("checksum-asset-mismatch", parsed.error.c_str());
}

void test_parser_rejects_invalid_hash_format() {
  const std::string payload =
      "not-a-real-hash  mylamp-c3-cylinder32x16-v0.2.0-release.bin\n";

  const lamp::update::ParsedChecksumFile parsed = lamp::update::parseChecksumFile(
      payload, "mylamp-c3-cylinder32x16-v0.2.0-release.bin");

  TEST_ASSERT_FALSE(parsed.valid);
  TEST_ASSERT_EQUAL_STRING("invalid-checksum-format", parsed.error.c_str());
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_parser_reads_sha256_sidecar_for_matching_asset);
  RUN_TEST(test_parser_rejects_mismatched_asset_name);
  RUN_TEST(test_parser_rejects_invalid_hash_format);
  return UNITY_END();
}