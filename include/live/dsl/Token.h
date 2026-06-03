#pragma once

#include <stdint.h>

#include <string>

namespace lamp::live::dsl {

enum class TokenType {
  kKeywordEffect,
  kKeywordSprite,
  kKeywordLayer,
  kKeywordBitmap,
  kKeywordUse,
  kKeywordColor,
  kKeywordX,
  kKeywordY,
  kKeywordScale,
  kKeywordRotation,
  kKeywordBlend,
  kKeywordVisible,
  kKeywordZ,
  kKeywordText,
  kKeywordFor,
  kKeywordFrame,
  kKeywordPalette,
  kKeywordClock,
  kIdentifier,
  kString,
  kMultilineString,
  kExpression,
  kLeftBrace,
  kRightBrace,
  kEquals,
  kSemicolon,
  kNewline,
  kEof,
  kUnknown,
};

struct Token {
  TokenType type = TokenType::kUnknown;
  std::string text;
  uint32_t line = 0;
  uint32_t column = 0;
};

}  // namespace lamp::live::dsl