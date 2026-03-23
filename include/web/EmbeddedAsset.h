#pragma once

#include <stddef.h>
#include <stdint.h>

namespace lamp::web {

struct EmbeddedAsset {
  const uint8_t* data;
  size_t length;
  const char* contentType;
  const char* contentEncoding;
};

static constexpr char kGzipContentEncoding[] = "gzip";

inline EmbeddedAsset makeEmbeddedAsset(const uint8_t* data, size_t length, const char* contentType,
                                       const char* contentEncoding = nullptr) {
  return EmbeddedAsset{data, length, contentType, contentEncoding};
}

inline EmbeddedAsset makeCompressedTextAsset(const uint8_t* data, size_t length,
                                             const char* contentType) {
  return makeEmbeddedAsset(data, length, contentType, kGzipContentEncoding);
}

inline bool isCompressedAsset(const EmbeddedAsset& asset) {
  return asset.contentEncoding != nullptr && asset.contentEncoding[0] != '\0';
}

}  // namespace lamp::web