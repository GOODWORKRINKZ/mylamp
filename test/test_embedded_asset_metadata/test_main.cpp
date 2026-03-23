#include <unity.h>

#include "web/EmbeddedAsset.h"

namespace {

void test_compressed_text_asset_uses_gzip_encoding() {
  static const uint8_t payload[] = {0x1f, 0x8b, 0x08, 0x00};

  const lamp::web::EmbeddedAsset asset =
      lamp::web::makeCompressedTextAsset(payload, sizeof(payload), "text/html; charset=utf-8");

  TEST_ASSERT_TRUE(lamp::web::isCompressedAsset(asset));
  TEST_ASSERT_EQUAL_STRING("gzip", asset.contentEncoding);
  TEST_ASSERT_EQUAL_STRING("text/html; charset=utf-8", asset.contentType);
  TEST_ASSERT_EQUAL_UINT32(sizeof(payload), static_cast<uint32_t>(asset.length));
}

void test_uncompressed_asset_has_no_content_encoding() {
  static const uint8_t payload[] = {0x7b, 0x7d};

  const lamp::web::EmbeddedAsset asset =
      lamp::web::makeEmbeddedAsset(payload, sizeof(payload), "application/json");

  TEST_ASSERT_FALSE(lamp::web::isCompressedAsset(asset));
  TEST_ASSERT_NULL(asset.contentEncoding);
  TEST_ASSERT_EQUAL_STRING("application/json", asset.contentType);
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_compressed_text_asset_uses_gzip_encoding);
  RUN_TEST(test_uncompressed_asset_has_no_content_encoding);
  return UNITY_END();
}