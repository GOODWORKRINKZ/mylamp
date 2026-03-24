#pragma once

#include <stdint.h>

#include <cstddef>
#include <string>

namespace lamp::live::runtime {

static constexpr int16_t kFontGlyphWidth = 3;
static constexpr int16_t kFontGlyphHeight = 5;
static constexpr int16_t kFontCharSpacing = 1;

struct FontGlyph {
  uint8_t rows[5];
};

const FontGlyph* getFontGlyph(uint16_t codepoint);

uint16_t decodeUtf8Char(const std::string& text, size_t& position);

}  // namespace lamp::live::runtime
