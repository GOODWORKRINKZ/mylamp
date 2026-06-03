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
  if (!startsWith(line, keyword + " ")) return false;
  std::string r = trim(line.substr(keyword.length()));
  if (r.empty()) return false;
  size_t b = r.find('{');
  if (b == std::string::npos) return false;
  name = trim(r.substr(0, b));
  return !name.empty();
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

    if (startsWith(trimmed, "text ")) {
      const std::string rest = trim(trimmed.substr(5));
      const std::string::size_type quoteStart = rest.find('"');
      const std::string::size_type quoteEnd = rest.rfind('"');
      if (quoteStart != std::string::npos && quoteEnd != std::string::npos && quoteEnd > quoteStart) {
        const std::string name = trim(rest.substr(0, quoteStart));
        const std::string content = rest.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
        if (!name.empty()) {
          appendToken(tokens, TokenType::kKeywordText, "text", lineNumber, 1U);
          appendToken(tokens, TokenType::kIdentifier, name, lineNumber, 6U);
          appendToken(tokens, TokenType::kString, content, lineNumber,
                      static_cast<uint32_t>(trimmed.find('"') + 2U));
          appendToken(tokens, TokenType::kNewline, "", lineNumber, 1U);
          continue;
        }
      }
    }

    if (parseBlockHeader(trimmed, "sprite", value)) {
      // value may be "mario" or "mario palette mario_pal"
      std::string spriteName = value;
      std::string paletteName;

      size_t palPos = spriteName.find(" palette ");
      if (palPos != std::string::npos) {
        paletteName = trim(spriteName.substr(palPos + 9));
        spriteName = trim(spriteName.substr(0, palPos));
      }

      appendToken(tokens, TokenType::kKeywordSprite, "sprite", lineNumber, 1U);
      appendToken(tokens, TokenType::kIdentifier, spriteName, lineNumber, 8U);
      if (!paletteName.empty()) {
        appendToken(tokens, TokenType::kKeywordPalette, "palette", lineNumber,
                    static_cast<uint32_t>(trimmed.find("palette") + 1U));
        appendToken(tokens, TokenType::kIdentifier, paletteName, lineNumber,
                    static_cast<uint32_t>(trimmed.find(paletteName) + 1U));
      }
      appendToken(tokens, TokenType::kLeftBrace, "{", lineNumber,
                  static_cast<uint32_t>(trimmed.find('{') + 1U));
      appendToken(tokens, TokenType::kNewline, "", lineNumber, 1U);

      // Scan ahead to determine sprite style: single-bitmap or multi-frame
      size_t peekIndex = index + 1;
      while (peekIndex < lines.size() && trim(lines[peekIndex]).empty()) {
        ++peekIndex;
      }
      std::string peekLine = (peekIndex < lines.size()) ? trim(lines[peekIndex]) : "";

      if (startsWith(peekLine, "frame ")) {
        // Multi-frame sprite mode
        int braceDepth = 1;  // We already consumed the opening {
        for (++index; index < lines.size() && braceDepth > 0; ++index) {
          const uint32_t frameLineNumber = static_cast<uint32_t>(index + 1U);
          const std::string frameTrimmed = trim(lines[index]);
          if (frameTrimmed.empty()) {
            appendToken(tokens, TokenType::kNewline, "", frameLineNumber, 1U);
            continue;
          }
          if (frameTrimmed == "}") {
            appendToken(tokens, TokenType::kRightBrace, "}", frameLineNumber, 1U);
            appendToken(tokens, TokenType::kNewline, "", frameLineNumber, 1U);
            --braceDepth;
            continue;
          }
          if (startsWith(frameTrimmed, "frame ")) {
            std::string frameName;
            if (!parseBlockHeader(frameTrimmed, "frame", frameName)) {
              diagnostics.push_back(makeDiagnostic(frameLineNumber, 1U,
                                                   "Ожидалось имя кадра после frame"));
              return false;
            }
            appendToken(tokens, TokenType::kKeywordFrame, "frame", frameLineNumber, 1U);
            appendToken(tokens, TokenType::kIdentifier, frameName, frameLineNumber, 7U);
            appendToken(tokens, TokenType::kLeftBrace, "{", frameLineNumber,
                        static_cast<uint32_t>(frameTrimmed.find('{') + 1U));
            appendToken(tokens, TokenType::kNewline, "", frameLineNumber, 1U);
            ++braceDepth;  // Account for frame's opening brace
            continue;
          }
          if (frameTrimmed == "bitmap \"\"\"") {
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
              diagnostics.push_back(makeDiagnostic(frameLineNumber, 1U,
                                                   "Не закрыт bitmap блок"));
              return false;
            }
            appendToken(tokens, TokenType::kKeywordBitmap, "bitmap", frameLineNumber, 1U);
            appendToken(tokens, TokenType::kMultilineString, bitmap, frameLineNumber, 8U);
            appendToken(tokens, TokenType::kNewline, "",
                        static_cast<uint32_t>(index + 1U), 1U);
            continue;
          }
          // Unknown content inside sprite body
          diagnostics.push_back(makeDiagnostic(frameLineNumber, 1U,
                                               "Неожиданный токен в sprite: " + frameTrimmed));
          return false;
        }
        // Outer loop ++index will move past the sprite closing }
      } else {
        // Single-bitmap path; also handle inline: sprite dot { bitmap """ # """ }
        size_t bp = trimmed.find('{');
        std::string ab = trim(trimmed.substr(bp + 1));
        if (startsWith(ab, "bitmap \"\"\"") && ab.size() > 10) {
          size_t eq = ab.rfind("\"\"\"", 10);
          if (eq != std::string::npos) {
            appendToken(tokens, TokenType::kKeywordBitmap, "bitmap", lineNumber, 1U);
            appendToken(tokens, TokenType::kMultilineString, trim(ab.substr(10, eq - 10)), lineNumber, 1U);
            appendToken(tokens, TokenType::kNewline, "", lineNumber, 1U);
            appendToken(tokens, TokenType::kRightBrace, "}", lineNumber, 1U);
            appendToken(tokens, TokenType::kNewline, "", lineNumber, 1U);
          }
        }
      }
      continue;
    }

    if (startsWith(trimmed, "clock {") || trimmed == "clock {") {
      appendToken(tokens, TokenType::kKeywordClock, "clock", lineNumber, 1U);
      appendToken(tokens, TokenType::kLeftBrace, "{", lineNumber,
                  static_cast<uint32_t>(trimmed.find('{') + 1U));
      appendToken(tokens, TokenType::kNewline, "", lineNumber, 1U);
      continue;
    }

    if (startsWith(trimmed, "palette ")) {
      std::string paletteName;
      if (!parseBlockHeader(trimmed, "palette", paletteName)) {
        diagnostics.push_back(makeDiagnostic(lineNumber, 1U,
                                             "Ожидалось имя палитры после palette"));
        return false;
      }
      appendToken(tokens, TokenType::kKeywordPalette, "palette", lineNumber, 1U);
      appendToken(tokens, TokenType::kIdentifier, paletteName, lineNumber, 9U);
      appendToken(tokens, TokenType::kLeftBrace, "{", lineNumber,
                  static_cast<uint32_t>(trimmed.find('{') + 1U));
      appendToken(tokens, TokenType::kNewline, "", lineNumber, 1U);

      // Parse palette body: <char> = <color expr> per line
      for (++index; index < lines.size(); ++index) {
        const uint32_t palLine = static_cast<uint32_t>(index + 1U);
        const std::string palTrimmed = trim(lines[index]);
        if (palTrimmed.empty()) {
          appendToken(tokens, TokenType::kNewline, "", palLine, 1U);
          continue;
        }
        if (palTrimmed == "}") {
          appendToken(tokens, TokenType::kRightBrace, "}", palLine, 1U);
          appendToken(tokens, TokenType::kNewline, "", palLine, 1U);
          break;
        }
        // Parse: <char> = <expression>
        if (palTrimmed.length() >= 3 && palTrimmed[1] == ' ' && palTrimmed[2] == '=') {
          char key = palTrimmed[0];
          appendToken(tokens, TokenType::kIdentifier, std::string(1, key), palLine,
                      static_cast<uint32_t>(trimmed.find(key) + 1U));
          appendToken(tokens, TokenType::kEquals, "=", palLine,
                      static_cast<uint32_t>(palTrimmed.find('=') + 1U));
          std::string expr = trim(palTrimmed.substr(3));
          appendToken(tokens, TokenType::kExpression, expr, palLine,
                      static_cast<uint32_t>(palTrimmed.find(expr) + 1U));
          appendToken(tokens, TokenType::kNewline, "", palLine, 1U);
        }
      }
      // Outer loop ++index will move past the palette closing }
      continue;
    }

    if (startsWith(trimmed, "for ")) {
      appendToken(tokens, TokenType::kKeywordFor, "for", lineNumber, 1U);

      // Parse for-header on this line: for i = 0; i < 3; i = i + 1 {
      std::string header = trim(trimmed.substr(4));  // after "for "
      // Check if header ends with { — if so, strip it
      bool hasBrace = false;
      if (!header.empty() && header.back() == '{') {
        header = trim(header.substr(0, header.length() - 1));
        hasBrace = true;
      }

      // Tokenize the for-header by splitting on semicolons
      std::string::size_type pos = 0;
      int clauseIndex = 0;
      std::string loopVarName;  // Track loop variable for step-clause stripping
      while (pos < header.length() && clauseIndex < 3) {
        std::string::size_type semi = header.find(';', pos);
        std::string clause;
        if (semi == std::string::npos) {
          clause = trim(header.substr(pos));
          pos = header.length();
        } else {
          clause = trim(header.substr(pos, semi - pos));
          pos = semi + 1;
        }

        // Tokenize each clause
        std::string::size_type eqPos = clause.find('=');
        if (eqPos != std::string::npos) {
          std::string varName = trim(clause.substr(0, eqPos));
          std::string expr = trim(clause.substr(eqPos + 1));
          if (clauseIndex == 0) loopVarName = varName;

          if (clauseIndex == 2 && startsWith(expr, loopVarName + " + ")) {
            // Step clause in form "i = i + N" → emit "i", "=", "i", "+", "N"
            std::string stepVal = trim(expr.substr(loopVarName.length() + 3));
            appendToken(tokens, TokenType::kIdentifier, varName, lineNumber,
                        static_cast<uint32_t>(trimmed.find(varName) + 1U));
            appendToken(tokens, TokenType::kEquals, "=", lineNumber,
                        static_cast<uint32_t>(trimmed.find('=') + 1U));
            appendToken(tokens, TokenType::kIdentifier, loopVarName, lineNumber,
                        static_cast<uint32_t>(trimmed.find(loopVarName) + 1U));
            appendToken(tokens, TokenType::kUnknown, "+", lineNumber,
                        static_cast<uint32_t>(trimmed.find('+') + 1U));
            appendToken(tokens, TokenType::kExpression, stepVal, lineNumber,
                        static_cast<uint32_t>(trimmed.find(stepVal) + 1U));
          } else {
            appendToken(tokens, TokenType::kIdentifier, varName, lineNumber,
                        static_cast<uint32_t>(trimmed.find(varName) + 1U));
            appendToken(tokens, TokenType::kEquals, "=", lineNumber,
                        static_cast<uint32_t>(trimmed.find('=') + 1U));
            appendToken(tokens, TokenType::kExpression, expr, lineNumber,
                        static_cast<uint32_t>(trimmed.find(expr) + 1U));
          }
        } else if (clauseIndex == 1) {
          // Middle clause: loopVar comparisonOp expression
          std::string op;
          std::string::size_type opPos = std::string::npos;
          const char* ops[] = {"<=", ">=", "<", ">"};
          for (const char* candidate : ops) {
            std::string::size_type found = clause.find(candidate);
            if (found != std::string::npos) {
              opPos = found;
              op = candidate;
              break;
            }
          }
          if (opPos != std::string::npos) {
            std::string leftVar = trim(clause.substr(0, opPos));
            std::string rightExpr = trim(clause.substr(opPos + op.length()));
            appendToken(tokens, TokenType::kIdentifier, leftVar, lineNumber,
                        static_cast<uint32_t>(trimmed.find(leftVar) + 1U));
            appendToken(tokens, TokenType::kUnknown, op, lineNumber,
                        static_cast<uint32_t>(trimmed.find(op) + 1U));
            appendToken(tokens, TokenType::kExpression, rightExpr, lineNumber,
                        static_cast<uint32_t>(trimmed.find(rightExpr) + 1U));
          }
        }
        ++clauseIndex;

        if (semi != std::string::npos) {
          appendToken(tokens, TokenType::kSemicolon, ";", lineNumber,
                      static_cast<uint32_t>(trimmed.find(';') + 1U));
        }
      }

      appendToken(tokens, TokenType::kNewline, "", lineNumber, 1U);
      if (hasBrace) {
        appendToken(tokens, TokenType::kLeftBrace, "{", lineNumber,
                    static_cast<uint32_t>(trimmed.find('{') + 1U));
        appendToken(tokens, TokenType::kNewline, "", lineNumber, 1U);
      }
      continue;
    }

    if (startsWith(trimmed, "layer ") && trimmed.back() == '{') {
      std::string value;
      if (parseBlockHeader(trimmed, "layer", value)) {
        appendToken(tokens, TokenType::kKeywordLayer, "layer", lineNumber, 1U);
        appendToken(tokens, TokenType::kIdentifier, value, lineNumber, 7U);
        appendToken(tokens, TokenType::kLeftBrace, "{", lineNumber,
                    static_cast<uint32_t>(trimmed.find('{') + 1U));
        appendToken(tokens, TokenType::kNewline, "", lineNumber, 1U);
      }
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
                         {"scale", TokenType::kKeywordScale},
                         {"rotation", TokenType::kKeywordRotation},
                         {"blend", TokenType::kKeywordBlend},
                         {"visible", TokenType::kKeywordVisible},
                         {"frame", TokenType::kKeywordFrame},
                         {"z", TokenType::kKeywordZ},
                         {"enabled", TokenType::kIdentifier},
                         {"alpha", TokenType::kIdentifier}};

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