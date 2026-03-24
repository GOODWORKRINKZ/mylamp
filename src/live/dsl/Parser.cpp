#include "live/dsl/Parser.h"

#include <vector>

#include "live/dsl/Lexer.h"

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

class ParserState {
 public:
  explicit ParserState(const std::vector<Token>& tokens) : tokens_(tokens) {}

  const Token& current() const { return tokens_[index_]; }

  bool match(TokenType type) {
    if (current().type != type) {
      return false;
    }
    ++index_;
    return true;
  }

  bool expect(TokenType type, const std::string& message,
              std::vector<lamp::live::Diagnostic>& diagnostics) {
    if (match(type)) {
      return true;
    }

    diagnostics.push_back(makeDiagnostic(current().line, current().column, message));
    return false;
  }

 private:
  const std::vector<Token>& tokens_;
  size_t index_ = 0;
};

void skipNewlines(ParserState& state) {
  while (state.match(TokenType::kNewline)) {
  }
}

bool parseSprite(ParserState& state, Program& program,
                 std::vector<lamp::live::Diagnostic>& diagnostics) {
  Token nameToken = state.current();
  if (!state.expect(TokenType::kIdentifier, "Ожидалось имя sprite", diagnostics) ||
      !state.expect(TokenType::kLeftBrace, "Ожидался символ { после sprite", diagnostics)) {
    return false;
  }

  skipNewlines(state);
  if (!state.expect(TokenType::kKeywordBitmap, "Ожидалось свойство bitmap", diagnostics)) {
    return false;
  }

  Token bitmapToken = state.current();
  if (!state.expect(TokenType::kMultilineString, "Ожидался bitmap блок", diagnostics)) {
    return false;
  }

  skipNewlines(state);
  if (!state.expect(TokenType::kRightBrace, "Ожидался символ } после sprite", diagnostics)) {
    return false;
  }

  SpriteDeclaration sprite;
  sprite.name = nameToken.text;
  sprite.bitmap = bitmapToken.text;
  program.sprites.push_back(sprite);
  skipNewlines(state);
  return true;
}

bool parseLayer(ParserState& state, Program& program,
                std::vector<lamp::live::Diagnostic>& diagnostics) {
  Token nameToken = state.current();
  if (!state.expect(TokenType::kIdentifier, "Ожидалось имя layer", diagnostics) ||
      !state.expect(TokenType::kLeftBrace, "Ожидался символ { после layer", diagnostics)) {
    return false;
  }

  LayerDeclaration layer;
  layer.name = nameToken.text;
  skipNewlines(state);

  while (state.current().type != TokenType::kRightBrace && state.current().type != TokenType::kEof) {
    switch (state.current().type) {
      case TokenType::kKeywordUse: {
        state.match(TokenType::kKeywordUse);
        Token valueToken = state.current();
        if (!state.expect(TokenType::kIdentifier, "Ожидалось имя sprite после use", diagnostics)) {
          return false;
        }
        layer.spriteName = valueToken.text;
        break;
      }
      case TokenType::kKeywordColor: {
        state.match(TokenType::kKeywordColor);
        Token valueToken = state.current();
        if (!state.expect(TokenType::kExpression, "Ожидалось выражение color", diagnostics)) {
          return false;
        }
        layer.colorExpression = valueToken.text;
        layer.colorLine = valueToken.line;
        break;
      }
      case TokenType::kKeywordX:
      case TokenType::kKeywordY:
      case TokenType::kKeywordScale:
      case TokenType::kKeywordRotation:
      case TokenType::kKeywordBlend:
      case TokenType::kKeywordVisible: {
        const TokenType propertyType = state.current().type;
        state.match(propertyType);
        if (!state.expect(TokenType::kEquals, "Ожидался символ =", diagnostics)) {
          return false;
        }
        Token valueToken = state.current();
        if (!state.expect(TokenType::kExpression, "Ожидалось выражение свойства", diagnostics)) {
          return false;
        }

        if (propertyType == TokenType::kKeywordX) {
          layer.xExpression = valueToken.text;
          layer.xLine = valueToken.line;
        } else if (propertyType == TokenType::kKeywordY) {
          layer.yExpression = valueToken.text;
          layer.yLine = valueToken.line;
        } else if (propertyType == TokenType::kKeywordScale) {
          layer.scaleExpression = valueToken.text;
          layer.scaleLine = valueToken.line;
        } else if (propertyType == TokenType::kKeywordRotation) {
          layer.rotationExpression = valueToken.text;
          layer.rotationLine = valueToken.line;
        } else if (propertyType == TokenType::kKeywordBlend) {
          layer.blendMode = valueToken.text;
          layer.blendLine = valueToken.line;
        } else {
          layer.visibleExpression = valueToken.text;
          layer.visibleLine = valueToken.line;
        }
        break;
      }
      case TokenType::kUnknown:
        diagnostics.push_back(makeDiagnostic(state.current().line, state.current().column,
                                             "Свойство слоя не поддерживается в v1: " +
                                                 state.current().text));
        return false;
      default:
        diagnostics.push_back(makeDiagnostic(state.current().line, state.current().column,
                                             "Неожиданный токен в layer"));
        return false;
    }

    skipNewlines(state);
  }

  if (!state.expect(TokenType::kRightBrace, "Ожидался символ } после layer", diagnostics)) {
    return false;
  }

  program.layers.push_back(layer);
  skipNewlines(state);
  return true;
}

}  // namespace

bool parseProgram(const std::string& source, Program& program,
                  std::vector<lamp::live::Diagnostic>& diagnostics) {
  Lexer lexer;
  std::vector<Token> tokens;
  if (!lexer.tokenize(source, tokens, diagnostics)) {
    return false;
  }

  ParserState state(tokens);
  skipNewlines(state);

  if (state.current().type != TokenType::kKeywordEffect) {
    diagnostics.push_back(makeDiagnostic(state.current().line, state.current().column,
                                         "Ожидалось объявление effect"));
    return false;
  }

  state.match(TokenType::kKeywordEffect);
  Token effectNameToken = state.current();
  if (!state.expect(TokenType::kString, "Ожидалось имя effect", diagnostics)) {
    return false;
  }

  Program parsedProgram;
  parsedProgram.effectName = effectNameToken.text;
  skipNewlines(state);

  while (state.current().type != TokenType::kEof) {
    if (state.current().type == TokenType::kKeywordSprite) {
      state.match(TokenType::kKeywordSprite);
      if (!parseSprite(state, parsedProgram, diagnostics)) {
        return false;
      }
      continue;
    }

    if (state.current().type == TokenType::kKeywordLayer) {
      state.match(TokenType::kKeywordLayer);
      if (!parseLayer(state, parsedProgram, diagnostics)) {
        return false;
      }
      continue;
    }

    if (state.current().type == TokenType::kUnknown) {
      diagnostics.push_back(makeDiagnostic(state.current().line, state.current().column,
                                           "Неожиданная конструкция DSL: " + state.current().text));
      return false;
    }

    skipNewlines(state);
    if (state.current().type != TokenType::kEof && state.current().type != TokenType::kKeywordSprite &&
        state.current().type != TokenType::kKeywordLayer) {
      diagnostics.push_back(makeDiagnostic(state.current().line, state.current().column,
                                           "Ожидалось объявление sprite или layer"));
      return false;
    }
  }

  program = parsedProgram;
  return true;
}

}  // namespace lamp::live::dsl