#include "live/dsl/Lexer.h"

#include <sstream>
#include <vector>

namespace lamp::live::dsl {

namespace {

lamp::live::Diagnostic makeDiagnostic(uint32_t line, uint32_t column,
                                      const std::string& message) {
  lamp::live::Diagnostic diagnostic;
  diagnostic.line = line;
  diagnostic.column = column;
  diagnostic.message = message;
  return diagnostic;
}

std::string trim(const std::string& text) {
  const std::string whitespace = " \t\r";
  const std::string::size_type start = text.find_first_not_of(whitespace);
  if (start == std::string::npos) {
    return "";
  }

  const std::string::size_type end = text.find_last_not_of(whitespace);
  return text.substr(start, end - start + 1);
}

void appendToken(std::vector<Token>& tokens, TokenType type, const std::string& text, uint32_t line,
                 uint32_t column) {
  Token token;
  token.type = type;
  token.text = text;
  token.line = line;
  token.column = column;
  tokens.push_back(token);
}

bool startsWith(const std::string& text, const std::string& prefix) {
  return text.rfind(prefix, 0) == 0;
}

bool parseQuotedString(const std::string& line, const std::string& prefix, std::string& value) {
  if (!startsWith(line, prefix + " \"")) {
    return false;
  }

  const std::string::size_type firstQuote = line.find('"');
  const std::string::size_type lastQuote = line.find_last_of('"');
  if (firstQuote == std::string::npos || lastQuote == std::string::npos || lastQuote <= firstQuote) {
    return false;
  }

  value = line.substr(firstQuote + 1, lastQuote - firstQuote - 1);
  return true;
}

bool parseBlockHeader(const std::string& line, const std::string& keyword, std::string& name) {
  if (!startsWith(line, keyword + " ") || line.empty() || line.back() != '{') {
    return false;
  }

  std::string remainder = trim(line.substr(keyword.length()));
  if (remainder.empty() || remainder.back() != '{') {
    return false;
  }

  remainder = trim(remainder.substr(0, remainder.length() - 1));
  if (remainder.empty()) {
    return false;
  }

  name = remainder;
  return true;
}

}  // namespace

bool Lexer::tokenize(const std::string& source, std::vector<Token>& tokens,
                     std::vector<lamp::live::Diagnostic>& diagnostics) const {
  std::vector<std::string> lines;
  std::stringstream stream(source);
  std::string line;
  while (std::getline(stream, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    lines.push_back(line);
  }

  for (size_t index = 0; index < lines.size(); ++index) {
    const uint32_t lineNumber = static_cast<uint32_t>(index + 1U);
    const std::string trimmed = trim(lines[index]);
    if (trimmed.empty()) {
      appendToken(tokens, TokenType::kNewline, "", lineNumber, 1U);
      continue;
    }

    std::string value;
    if (parseQuotedString(trimmed, "effect", value)) {
      appendToken(tokens, TokenType::kKeywordEffect, "effect", lineNumber, 1U);
      appendToken(tokens, TokenType::kString, value, lineNumber, 8U);
      appendToken(tokens, TokenType::kNewline, "", lineNumber, 1U);
      continue;
    }

    if (parseBlockHeader(trimmed, "sprite", value)) {
      appendToken(tokens, TokenType::kKeywordSprite, "sprite", lineNumber, 1U);
      appendToken(tokens, TokenType::kIdentifier, value, lineNumber, 8U);
      appendToken(tokens, TokenType::kLeftBrace, "{", lineNumber,
                  static_cast<uint32_t>(trimmed.find('{') + 1U));
      appendToken(tokens, TokenType::kNewline, "", lineNumber, 1U);
      continue;
    }

    if (parseBlockHeader(trimmed, "layer", value)) {
      appendToken(tokens, TokenType::kKeywordLayer, "layer", lineNumber, 1U);
      appendToken(tokens, TokenType::kIdentifier, value, lineNumber, 7U);
      appendToken(tokens, TokenType::kLeftBrace, "{", lineNumber,
                  static_cast<uint32_t>(trimmed.find('{') + 1U));
      appendToken(tokens, TokenType::kNewline, "", lineNumber, 1U);
      continue;
    }

    if (trimmed == "}") {
      appendToken(tokens, TokenType::kRightBrace, "}", lineNumber, 1U);
      appendToken(tokens, TokenType::kNewline, "", lineNumber, 1U);
      continue;
    }

    if (trimmed == "bitmap \"\"\"") {
      std::string bitmap;
      bool closed = false;
      for (++index; index < lines.size(); ++index) {
        const std::string bitmapLine = lines[index];
        if (trim(bitmapLine) == "\"\"\"") {
          closed = true;
          break;
        }

        if (!bitmap.empty()) {
          bitmap += '\n';
        }
        bitmap += trim(bitmapLine);
      }

      if (!closed) {
        diagnostics.push_back(makeDiagnostic(lineNumber, 1U, "Не закрыт bitmap блок"));
        return false;
      }

      appendToken(tokens, TokenType::kKeywordBitmap, "bitmap", lineNumber, 1U);
      appendToken(tokens, TokenType::kMultilineString, bitmap, lineNumber, 8U);
      appendToken(tokens, TokenType::kNewline, "", static_cast<uint32_t>(index + 1U), 1U);
      continue;
    }

    if (startsWith(trimmed, "use ")) {
      appendToken(tokens, TokenType::kKeywordUse, "use", lineNumber, 1U);
      appendToken(tokens, TokenType::kIdentifier, trim(trimmed.substr(4)), lineNumber, 5U);
      appendToken(tokens, TokenType::kNewline, "", lineNumber, 1U);
      continue;
    }

    if (startsWith(trimmed, "color ")) {
      appendToken(tokens, TokenType::kKeywordColor, "color", lineNumber, 1U);
      appendToken(tokens, TokenType::kExpression, trim(trimmed.substr(6)), lineNumber, 7U);
      appendToken(tokens, TokenType::kNewline, "", lineNumber, 1U);
      continue;
    }

    const struct PropertyRule {
      const char* keyword;
      TokenType tokenType;
    } propertyRules[] = {{"x", TokenType::kKeywordX},       {"y", TokenType::kKeywordY},
                         {"scale", TokenType::kKeywordScale}, {"visible", TokenType::kKeywordVisible}};

    bool matchedProperty = false;
    for (const PropertyRule& rule : propertyRules) {
      const std::string prefix = std::string(rule.keyword) + " = ";
      if (!startsWith(trimmed, prefix)) {
        continue;
      }

      appendToken(tokens, rule.tokenType, rule.keyword, lineNumber, 1U);
      appendToken(tokens, TokenType::kEquals, "=", lineNumber,
                  static_cast<uint32_t>(trimmed.find('=') + 1U));
      appendToken(tokens, TokenType::kExpression, trim(trimmed.substr(prefix.length())), lineNumber,
                  static_cast<uint32_t>(prefix.length() + 1U));
      appendToken(tokens, TokenType::kNewline, "", lineNumber, 1U);
      matchedProperty = true;
      break;
    }
    if (matchedProperty) {
      continue;
    }

    const std::string::size_type spaceIndex = trimmed.find(' ');
    const std::string keyword = spaceIndex == std::string::npos ? trimmed : trimmed.substr(0, spaceIndex);
    appendToken(tokens, TokenType::kUnknown, keyword, lineNumber, 1U);
    appendToken(tokens, TokenType::kNewline, "", lineNumber, 1U);
  }

  appendToken(tokens, TokenType::kEof, "", static_cast<uint32_t>(lines.size() + 1U), 1U);
  return true;
}

}  // namespace lamp::live::dsl