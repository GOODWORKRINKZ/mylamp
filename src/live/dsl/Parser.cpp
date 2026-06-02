#include "live/dsl/Parser.h"

#include <unordered_set>
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
  if (!state.expect(TokenType::kIdentifier, "Ожидалось имя sprite", diagnostics)) {
    return false;
  }

  SpriteDeclaration sprite;
  sprite.name = nameToken.text;

  // Optional palette reference: sprite name palette palname {
  if (state.current().type == TokenType::kKeywordPalette) {
    state.match(TokenType::kKeywordPalette);
    Token palToken = state.current();
    if (!state.expect(TokenType::kIdentifier, "Ожидалось имя палитры после palette", diagnostics)) {
      return false;
    }
    sprite.paletteName = palToken.text;
  }

  if (!state.expect(TokenType::kLeftBrace, "Ожидался символ { после sprite", diagnostics)) {
    return false;
  }

  skipNewlines(state);

  // Detect sprite style: single-bitmap (kKeywordBitmap) or multi-frame (kKeywordFrame)
  if (state.current().type == TokenType::kKeywordFrame) {
    // Multi-frame sprite mode
    while (state.current().type == TokenType::kKeywordFrame) {
      state.match(TokenType::kKeywordFrame);
      Token frameName = state.current();
      if (!state.expect(TokenType::kIdentifier, "Ожидалось имя кадра", diagnostics)) {
        return false;
      }
      if (!state.expect(TokenType::kLeftBrace, "Ожидался символ { после имени кадра", diagnostics)) {
        return false;
      }
      skipNewlines(state);
      if (!state.expect(TokenType::kKeywordBitmap, "Ожидалось свойство bitmap в кадре", diagnostics)) {
        return false;
      }
      Token bitmapToken = state.current();
      if (!state.expect(TokenType::kMultilineString, "Ожидался bitmap блок кадра", diagnostics)) {
        return false;
      }
      skipNewlines(state);
      if (!state.expect(TokenType::kRightBrace, "Ожидался символ } после кадра", diagnostics)) {
        return false;
      }
      SpriteFrameDeclaration frame;
      frame.name = frameName.text;
      frame.bitmap = bitmapToken.text;
      sprite.frames.push_back(frame);
      skipNewlines(state);
    }
    // sprite.bitmap stays empty — frames vector drives rendering
  } else {
    // Single-bitmap path (backward compat, D-04)
    if (!state.expect(TokenType::kKeywordBitmap, "Ожидалось свойство bitmap", diagnostics)) {
      return false;
    }
    Token bitmapToken = state.current();
    if (!state.expect(TokenType::kMultilineString, "Ожидался bitmap блок", diagnostics)) {
      return false;
    }
    sprite.bitmap = bitmapToken.text;
    skipNewlines(state);
  }

  if (!state.expect(TokenType::kRightBrace, "Ожидался символ } после sprite", diagnostics)) {
    return false;
  }

  program.sprites.push_back(sprite);
  skipNewlines(state);
  return true;
}

bool parseText(ParserState& state, Program& program,
               std::vector<lamp::live::Diagnostic>& diagnostics) {
  Token nameToken = state.current();
  if (!state.expect(TokenType::kIdentifier, "Ожидалось имя text", diagnostics)) {
    return false;
  }

  Token contentToken = state.current();
  if (!state.expect(TokenType::kString, "Ожидалась строка text", diagnostics)) {
    return false;
  }

  TextDeclaration text;
  text.name = nameToken.text;
  text.content = contentToken.text;
  program.texts.push_back(text);
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
      case TokenType::kKeywordVisible:
      case TokenType::kKeywordFrame:
      case TokenType::kKeywordZ: {
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
        } else if (propertyType == TokenType::kKeywordFrame) {
          layer.frameExpression = valueToken.text;
          layer.frameLine = valueToken.line;
        } else if (propertyType == TokenType::kKeywordZ) {
          layer.zExpression = valueToken.text;
          layer.zLine = valueToken.line;
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

void skipToNextTopLevel(ParserState& state) {
  while (state.current().type != TokenType::kEof &&
         state.current().type != TokenType::kKeywordSprite &&
         state.current().type != TokenType::kKeywordText &&
         state.current().type != TokenType::kKeywordLayer &&
         state.current().type != TokenType::kKeywordFor) {
    // Consume tokens until next top-level construct
    state.match(state.current().type);
  }
}

bool parseForLoop(ParserState& state, Program& program,
                  std::vector<lamp::live::Diagnostic>& diagnostics) {
  // for keyword already consumed by caller (parseProgram)

  // Parse loop variable
  Token loopVar = state.current();
  if (!state.expect(TokenType::kIdentifier, "Ожидалось имя переменной цикла", diagnostics)) {
    return false;
  }

  // Reserved word check
  static const std::unordered_set<std::string> kReservedWords = {
    "t", "dt", "temp", "humidity",
    "x", "y", "nx", "ny",
    "sin", "cos", "abs", "min", "max", "clamp", "mix", "smoothstep",
    "rgb", "hsv"
  };
  if (kReservedWords.count(loopVar.text)) {
    diagnostics.push_back(makeDiagnostic(loopVar.line, loopVar.column,
      "Имя переменной цикла не может совпадать со встроенным: " + loopVar.text));
    skipToNextTopLevel(state);
    return false;
  }

  // Parse start value: = expression
  if (!state.expect(TokenType::kEquals, "Ожидался символ =", diagnostics)) {
    return false;
  }
  Token startExpr = state.current();
  if (!state.expect(TokenType::kExpression, "Ожидалось начальное значение цикла", diagnostics)) {
    return false;
  }

  // Optional semicolon
  state.match(TokenType::kSemicolon);

  // Parse condition: loopVar comparisonOp expression
  Token condVar = state.current();
  if (!state.expect(TokenType::kIdentifier, "Ожидалось имя переменной цикла", diagnostics)) {
    return false;
  }
  if (condVar.text != loopVar.text) {
    diagnostics.push_back(makeDiagnostic(condVar.line, condVar.column,
      "Имя переменной цикла должно совпадать: " + loopVar.text));
    return false;
  }

  // Comparison operator (tokenized as kUnknown with the op text)
  std::string compOp = state.current().text;
  if (compOp == "<" || compOp == "<=" || compOp == ">" || compOp == ">=") {
    state.match(TokenType::kUnknown);
  } else {
    diagnostics.push_back(makeDiagnostic(state.current().line, state.current().column,
      "Ожидался оператор сравнения (<, <=, >, >=)"));
    return false;
  }

  // End bound expression
  Token endExpr = state.current();
  if (!state.expect(TokenType::kExpression, "Ожидалось конечное значение цикла", diagnostics)) {
    return false;
  }

  // Optional semicolon
  state.match(TokenType::kSemicolon);

  // Parse step: loopVar = expression (the expression may contain the loop var, e.g. "i + 1")
  Token stepVar1 = state.current();
  if (!state.expect(TokenType::kIdentifier, "Ожидалось имя переменной цикла", diagnostics)) {
    return false;
  }
  if (stepVar1.text != loopVar.text) {
    diagnostics.push_back(makeDiagnostic(stepVar1.line, stepVar1.column,
      "Имя переменной цикла должно совпадать: " + loopVar.text));
    return false;
  }
  if (!state.expect(TokenType::kEquals, "Ожидался символ =", diagnostics)) {
    return false;
  }

  // Step expression: may be simple "1" or compound "i + 1"
  Token stepExpr = state.current();
  if (state.current().type == TokenType::kIdentifier &&
      state.current().text == loopVar.text) {
    // Compound form: i = i + N → consume i, +, N
    state.match(TokenType::kIdentifier);
    state.match(TokenType::kUnknown);  // consume '+'
    stepExpr = state.current();
  }
  if (!state.expect(TokenType::kExpression, "Ожидался шаг цикла", diagnostics)) {
    return false;
  }

  // Loop bound validation: must be integer constants
  auto isInteger = [](const std::string& s) -> bool {
    if (s.empty()) return false;
    size_t start = (s[0] == '-') ? 1 : 0;
    if (start >= s.length()) return false;
    for (size_t i = start; i < s.length(); ++i) {
      if (s[i] < '0' || s[i] > '9') return false;
    }
    return true;
  };
  if (!isInteger(startExpr.text) || !isInteger(endExpr.text) || !isInteger(stepExpr.text)) {
    diagnostics.push_back(makeDiagnostic(startExpr.line, startExpr.column,
      "Границы цикла for должны быть целыми числами"));
    return false;
  }

  // Consume opening brace (may be on next line)
  skipNewlines(state);
  if (!state.expect(TokenType::kLeftBrace, "Ожидался символ { после заголовка for", diagnostics)) {
    return false;
  }

  // Parse body: sequence of layer declarations
  ForLoopStatement forLoop;
  forLoop.loopVariable = loopVar.text;
  forLoop.startExpression = startExpr.text;
  forLoop.endExpression = endExpr.text;
  forLoop.stepExpression = stepExpr.text;
  forLoop.comparisonOperator = compOp;

  skipNewlines(state);
  while (state.current().type != TokenType::kRightBrace && state.current().type != TokenType::kEof) {
    if (state.current().type == TokenType::kKeywordLayer) {
      state.match(TokenType::kKeywordLayer);
      LayerDeclaration bodyLayer;
      // Temporarily redirect layer parsing into forLoop.body
      // We need to parse a layer but add it to forLoop.body, not program.layers
      // Reuse parseLayer logic inline
      Token layerName = state.current();
      if (!state.expect(TokenType::kIdentifier, "Ожидалось имя layer", diagnostics) ||
          !state.expect(TokenType::kLeftBrace, "Ожидался символ { после layer", diagnostics)) {
        return false;
      }
      bodyLayer.name = layerName.text;
      skipNewlines(state);

      while (state.current().type != TokenType::kRightBrace && state.current().type != TokenType::kEof) {
        switch (state.current().type) {
          case TokenType::kKeywordUse: {
            state.match(TokenType::kKeywordUse);
            Token valueToken = state.current();
            if (!state.expect(TokenType::kIdentifier, "Ожидалось имя sprite после use", diagnostics)) {
              return false;
            }
            bodyLayer.spriteName = valueToken.text;
            break;
          }
          case TokenType::kKeywordColor: {
            state.match(TokenType::kKeywordColor);
            Token valueToken = state.current();
            if (!state.expect(TokenType::kExpression, "Ожидалось выражение color", diagnostics)) {
              return false;
            }
            bodyLayer.colorExpression = valueToken.text;
            bodyLayer.colorLine = valueToken.line;
            break;
          }
          case TokenType::kKeywordX:
          case TokenType::kKeywordY:
          case TokenType::kKeywordScale:
          case TokenType::kKeywordRotation:
          case TokenType::kKeywordBlend:
          case TokenType::kKeywordVisible:
          case TokenType::kKeywordFrame:
          case TokenType::kKeywordZ: {
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
              bodyLayer.xExpression = valueToken.text;
              bodyLayer.xLine = valueToken.line;
            } else if (propertyType == TokenType::kKeywordY) {
              bodyLayer.yExpression = valueToken.text;
              bodyLayer.yLine = valueToken.line;
            } else if (propertyType == TokenType::kKeywordScale) {
              bodyLayer.scaleExpression = valueToken.text;
              bodyLayer.scaleLine = valueToken.line;
            } else if (propertyType == TokenType::kKeywordRotation) {
              bodyLayer.rotationExpression = valueToken.text;
              bodyLayer.rotationLine = valueToken.line;
            } else if (propertyType == TokenType::kKeywordBlend) {
              bodyLayer.blendMode = valueToken.text;
              bodyLayer.blendLine = valueToken.line;
            } else if (propertyType == TokenType::kKeywordFrame) {
              bodyLayer.frameExpression = valueToken.text;
              bodyLayer.frameLine = valueToken.line;
            } else if (propertyType == TokenType::kKeywordZ) {
              bodyLayer.zExpression = valueToken.text;
              bodyLayer.zLine = valueToken.line;
            } else {
              bodyLayer.visibleExpression = valueToken.text;
              bodyLayer.visibleLine = valueToken.line;
            }
            break;
          }
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
      forLoop.body.push_back(bodyLayer);
      skipNewlines(state);
    } else {
      break;
    }
  }

  if (!state.expect(TokenType::kRightBrace, "Ожидался символ } после тела for", diagnostics)) {
    return false;
  }

  program.forLoops.push_back(forLoop);
  skipNewlines(state);
  return true;
}

bool parsePalette(ParserState& state, Program& program,
                  std::vector<lamp::live::Diagnostic>& diagnostics) {
  // palette keyword already consumed by caller (parseProgram)
  Token nameToken = state.current();
  if (!state.expect(TokenType::kIdentifier, "Ожидалось имя палитры", diagnostics) ||
      !state.expect(TokenType::kLeftBrace, "Ожидался символ { после имени палитры", diagnostics)) {
    return false;
  }

  PaletteDeclaration palette;
  palette.name = nameToken.text;
  skipNewlines(state);

  while (state.current().type != TokenType::kRightBrace && state.current().type != TokenType::kEof) {
    // Each entry: <single-char identifier> = <color expression>
    Token keyToken = state.current();
    if (!state.expect(TokenType::kIdentifier, "Ожидался символ палитры", diagnostics)) {
      return false;
    }
    if (keyToken.text.length() != 1) {
      diagnostics.push_back(makeDiagnostic(keyToken.line, keyToken.column,
        "Ключ палитры должен быть одним символом: " + keyToken.text));
      return false;
    }
    char key = keyToken.text[0];

    if (!state.expect(TokenType::kEquals, "Ожидался символ =", diagnostics)) {
      return false;
    }

    Token colorToken = state.current();
    if (!state.expect(TokenType::kExpression, "Ожидалось выражение цвета", diagnostics)) {
      return false;
    }

    PaletteEntry entry;
    entry.key = key;
    entry.colorExpression = colorToken.text;
    palette.entries.push_back(entry);

    skipNewlines(state);
  }

  if (!state.expect(TokenType::kRightBrace, "Ожидался символ } после палитры", diagnostics)) {
    return false;
  }

  program.palettes.push_back(palette);
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
    if (state.current().type == TokenType::kKeywordPalette) {
      state.match(TokenType::kKeywordPalette);
      if (!parsePalette(state, parsedProgram, diagnostics)) {
        return false;
      }
      continue;
    }

    if (state.current().type == TokenType::kKeywordSprite) {
      state.match(TokenType::kKeywordSprite);
      if (!parseSprite(state, parsedProgram, diagnostics)) {
        return false;
      }
      continue;
    }

    if (state.current().type == TokenType::kKeywordText) {
      state.match(TokenType::kKeywordText);
      if (!parseText(state, parsedProgram, diagnostics)) {
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

    if (state.current().type == TokenType::kKeywordFor) {
      state.match(TokenType::kKeywordFor);
      if (!parseForLoop(state, parsedProgram, diagnostics)) {
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
        state.current().type != TokenType::kKeywordLayer &&
        state.current().type != TokenType::kKeywordText &&
        state.current().type != TokenType::kKeywordFor &&
        state.current().type != TokenType::kKeywordPalette) {
      diagnostics.push_back(makeDiagnostic(state.current().line, state.current().column,
                                           "Ожидалось объявление palette, sprite, text, layer или for"));
      return false;
    }
  }

  program = parsedProgram;
  return true;
}

}  // namespace lamp::live::dsl