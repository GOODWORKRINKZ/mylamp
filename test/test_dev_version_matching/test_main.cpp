#include <unity.h>

#include <string>

#include "update/GitHubReleaseParser.h"

namespace {

void test_parser_does_not_offer_same_dev_release_when_current_version_lacks_prefix() {
  const std::string payload = R"json([
    {
      "tag_name": "dev-develop-a1b2c3d-20260323-204500",
      "name": "dev build",
      "body": "Latest dev release",
      "draft": false,
      "prerelease": true,
      "published_at": "2026-03-23T20:45:00Z",
      "assets": [
        {
          "name": "mylamp-c3-cylinder32x16-dev-develop-a1b2c3d-20260323-204500.bin",
          "browser_download_url": "https://example.com/dev.bin"
        }
      ]
    }
  ])json";

  lamp::update::GitHubReleaseParser parser;
  const lamp::update::FirmwareReleaseInfo info =
      parser.parse(payload, "develop-a1b2c3d-20260323-204500", "dev", "c3-cylinder32x16");

  TEST_ASSERT_FALSE(info.available);
  TEST_ASSERT_EQUAL_STRING("", info.error.c_str());
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_parser_does_not_offer_same_dev_release_when_current_version_lacks_prefix);
  return UNITY_END();
}