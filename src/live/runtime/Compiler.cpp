#include "live/runtime/Compiler.h"

#include <ctype.h>

#include <cstdlib>
#include <string>
#include <vector>

namespace lamp::live::runtime {

namespace {

lamp::live::Diagnostic makeDiagnostic(uint32_t line, const std::string& message) {
  lamp::live::Diagnostic diagnostic;
  diagnostic.line = line;
  diagnostic.column = 0;
  diagnostic.message = message;
  return diagnostic;
}

class ExpressionCompiler {
 public:
  explicit ExpressionCompiler(std::vector<ExpressionNode>& nodes) : nodes_(nodes) {}

  bool compile(const std::string& expression, int16_t& rootIndex,
               std::vector<lamp::live::Diagnostic>& diagnostics, uint32_t sourceLine = 0) {
    input_ = expression;
    position_ = 0;
    sourceLine_ = sourceLine;
    skipWhitespace();
    if (!parseExpression(rootIndex, diagnostics)) {
      return false;
    }
    skipWhitespace();
    if (position_ != input_.size()) {
      diagnostics.push_back(makeDiagnostic(sourceLine_, "Не удалось разобрать выражение: " + expression));
      return false;
    }
    return true;
  }

 private:
  bool parseExpression(int16_t& index, std::vector<lamp::live::Diagnostic>& diagnostics) {
    if (!parseTerm(index, diagnostics)) {
      return false;
    }

    while (true) {
      skipWhitespace();
      if (!match('+') && !match('-')) {
        return true;
      }

      const char op = input_[position_ - 1U];
      int16_t rhs = -1;
      if (!parseTerm(rhs, diagnostics)) {
        return false;
      }

      ExpressionNode node;
      node.op = op == '+' ? ExpressionOp::kAdd : ExpressionOp::kSubtract;
      node.children[0] = index;
      node.children[1] = rhs;
      index = appendNode(node);
    }
  }

  bool parseTerm(int16_t& index, std::vector<lamp::live::Diagnostic>& diagnostics) {
    if (!parseUnary(index, diagnostics)) {
      return false;
    }

    while (true) {
      skipWhitespace();
      if (!match('*') && !match('/') && !match('%')) {
        return true;
      }

      const char op = input_[position_ - 1U];
      int16_t rhs = -1;
      if (!parseUnary(rhs, diagnostics)) {
        return false;
      }

      ExpressionNode node;
      if (op == '*') {
        node.op = ExpressionOp::kMultiply;
      } else if (op == '/') {
        node.op = ExpressionOp::kDivide;
      } else {
        node.op = ExpressionOp::kModulo;
      }
      node.children[0] = index;
      node.children[1] = rhs;
      index = appendNode(node);
    }
  }

  bool parseUnary(int16_t& index, std::vector<lamp::live::Diagnostic>& diagnostics) {
    skipWhitespace();
    if (match('-')) {
      int16_t child = -1;
      if (!parseUnary(child, diagnostics)) {
        return false;
      }

      ExpressionNode node;
      node.op = ExpressionOp::kNegate;
      node.children[0] = child;
      index = appendNode(node);
      return true;
    }

    return parsePrimary(index, diagnostics);
  }

  bool parsePrimary(int16_t& index, std::vector<lamp::live::Diagnostic>& diagnostics) {
    skipWhitespace();
    if (match('(')) {
      if (!parseExpression(index, diagnostics)) {
        return false;
      }
      skipWhitespace();
      if (!match(')')) {
        diagnostics.push_back(makeDiagnostic(sourceLine_, "Ожидалась закрывающая скобка"));
        return false;
      }
      return true;
    }

    if (position_ < input_.size() && (isdigit(static_cast<unsigned char>(input_[position_])) ||
                                      input_[position_] == '.')) {
      return parseNumber(index);
    }

    return parseIdentifier(index, diagnostics);
  }

  bool parseNumber(int16_t& index) {
    const size_t start = position_;
    while (position_ < input_.size() &&
           (isdigit(static_cast<unsigned char>(input_[position_])) || input_[position_] == '.')) {
      ++position_;
    }

    ExpressionNode node;
    node.op = ExpressionOp::kConstant;
    node.constant = static_cast<float>(std::atof(input_.substr(start, position_ - start).c_str()));
    index = appendNode(node);
    return true;
  }

  bool parseIdentifier(int16_t& index, std::vector<lamp::live::Diagnostic>& diagnostics) {
    skipWhitespace();
    const size_t start = position_;
    while (position_ < input_.size() &&
           (isalnum(static_cast<unsigned char>(input_[position_])) || input_[position_] == '_')) {
      ++position_;
    }

    if (start == position_) {
      diagnostics.push_back(makeDiagnostic(sourceLine_, "Ожидалось выражение"));
      return false;
    }

    const std::string name = input_.substr(start, position_ - start);
    skipWhitespace();
    if (match('(')) {
      std::vector<int16_t> arguments;
      skipWhitespace();
      if (!match(')')) {
        while (true) {
          int16_t argument = -1;
          if (!parseExpression(argument, diagnostics)) {
            return false;
          }
          arguments.push_back(argument);
          skipWhitespace();
          if (match(')')) {
            break;
          }
          if (!match(',')) {
            diagnostics.push_back(makeDiagnostic(sourceLine_, "Ожидалась запятая в списке аргументов"));
            return false;
          }
        }
      }

      return buildFunction(name, arguments, index, diagnostics);
    }

    ExpressionNode node;
    if (name == "t") {
      node.op = ExpressionOp::kTime;
    } else if (name == "dt") {
      node.op = ExpressionOp::kDeltaTime;
    } else if (name == "x") {
      node.op = ExpressionOp::kCoordX;
    } else if (name == "y") {
      node.op = ExpressionOp::kCoordY;
    } else if (name == "nx") {
      node.op = ExpressionOp::kCoordNx;
    } else if (name == "ny") {
      node.op = ExpressionOp::kCoordNy;
    } else {
      diagnostics.push_back(makeDiagnostic(sourceLine_, "Неизвестный идентификатор: " + name));
      return false;
    }

    index = appendNode(node);
    return true;
  }

  bool buildFunction(const std::string& name, const std::vector<int16_t>& arguments, int16_t& index,
                     std::vector<lamp::live::Diagnostic>& diagnostics) {
    ExpressionNode node;
    if (name == "temp") {
      if (!arguments.empty()) {
        diagnostics.push_back(makeDiagnostic(sourceLine_, "temp() не принимает аргументы"));
        return false;
      }
      node.op = ExpressionOp::kTemperature;
    } else if (name == "humidity") {
      if (!arguments.empty()) {
        diagnostics.push_back(makeDiagnostic(sourceLine_, "humidity() не принимает аргументы"));
        return false;
      }
      node.op = ExpressionOp::kHumidity;
    } else if (name == "sin" && arguments.size() == 1U) {
      node.op = ExpressionOp::kSin;
      node.children[0] = arguments[0];
    } else if (name == "cos" && arguments.size() == 1U) {
      node.op = ExpressionOp::kCos;
      node.children[0] = arguments[0];
    } else if (name == "abs" && arguments.size() == 1U) {
      node.op = ExpressionOp::kAbs;
      node.children[0] = arguments[0];
    } else if (name == "min" && arguments.size() == 2U) {
      node.op = ExpressionOp::kMin;
      node.children[0] = arguments[0];
      node.children[1] = arguments[1];
    } else if (name == "max" && arguments.size() == 2U) {
      node.op = ExpressionOp::kMax;
      node.children[0] = arguments[0];
      node.children[1] = arguments[1];
    } else if (name == "clamp" && arguments.size() == 3U) {
      node.op = ExpressionOp::kClamp;
      node.children[0] = arguments[0];
      node.children[1] = arguments[1];
      node.children[2] = arguments[2];
    } else if (name == "mix" && arguments.size() == 3U) {
      node.op = ExpressionOp::kMix;
      node.children[0] = arguments[0];
      node.children[1] = arguments[1];
      node.children[2] = arguments[2];
    } else if (name == "smoothstep" && arguments.size() == 3U) {
      node.op = ExpressionOp::kSmoothstep;
      node.children[0] = arguments[0];
      node.children[1] = arguments[1];
      node.children[2] = arguments[2];
    } else {
      diagnostics.push_back(makeDiagnostic(sourceLine_, "Неизвестная функция: " + name));
      return false;
    }

    index = appendNode(node);
    return true;
  }

  void skipWhitespace() {
    while (position_ < input_.size() &&
           (input_[position_] == ' ' || input_[position_] == '\t' || input_[position_] == '\r')) {
      ++position_;
    }
  }

  bool match(char ch) {
    if (position_ >= input_.size() || input_[position_] != ch) {
      return false;
    }
    ++position_;
    return true;
  }

  int16_t appendNode(const ExpressionNode& node) {
    nodes_.push_back(node);
    return static_cast<int16_t>(nodes_.size() - 1);
  }

  std::vector<ExpressionNode>& nodes_;
  std::string input_;
  size_t position_ = 0;
  uint32_t sourceLine_ = 0;
};

std::string trim(const std::string& text) {
  const std::string whitespace = " \t\r\n";
  const std::string::size_type start = text.find_first_not_of(whitespace);
  if (start == std::string::npos) {
    return "";
  }
  const std::string::size_type end = text.find_last_not_of(whitespace);
  return text.substr(start, end - start + 1U);
}

bool compileBlendMode(const std::string& modeText, BlendMode& blendMode,
                      std::vector<lamp::live::Diagnostic>& diagnostics, uint32_t sourceLine = 0) {
  const std::string trimmed = trim(modeText);
  if (trimmed.empty() || trimmed == "normal") {
    blendMode = BlendMode::kNormal;
    return true;
  }
  if (trimmed == "add") {
    blendMode = BlendMode::kAdd;
    return true;
  }
  if (trimmed == "multiply") {
    blendMode = BlendMode::kMultiply;
    return true;
  }

  diagnostics.push_back(makeDiagnostic(sourceLine, "Поддерживаются blend режимы: normal, add, multiply"));
  return false;
}

bool splitArguments(const std::string& text, std::vector<std::string>& arguments) {
  int depth = 0;
  size_t start = 0;
  for (size_t index = 0; index < text.size(); ++index) {
    const char ch = text[index];
    if (ch == '(') {
      ++depth;
    } else if (ch == ')') {
      --depth;
    } else if (ch == ',' && depth == 0) {
      arguments.push_back(trim(text.substr(start, index - start)));
      start = index + 1U;
    }
  }

  if (depth != 0) {
    return false;
  }

  arguments.push_back(trim(text.substr(start)));
  return true;
}

bool compileColor(const std::string& expression, std::vector<ExpressionNode>& nodes,
                  CompiledColor& color, std::vector<lamp::live::Diagnostic>& diagnostics,
                  uint32_t sourceLine = 0) {
  const std::string trimmed = trim(expression);
  if ((trimmed.rfind("rgb(", 0) != 0 && trimmed.rfind("hsv(", 0) != 0) || trimmed.back() != ')') {
    diagnostics.push_back(makeDiagnostic(sourceLine, "Поддерживаются только rgb(...) и hsv(...)"));
    return false;
  }

  color.model = trimmed.rfind("rgb(", 0) == 0 ? ColorModel::kRgb : ColorModel::kHsv;
  const std::string argsText = trimmed.substr(4, trimmed.size() - 5U);
  std::vector<std::string> arguments;
  if (!splitArguments(argsText, arguments) || arguments.size() != 3U) {
    diagnostics.push_back(makeDiagnostic(sourceLine, "Ожидалось три аргумента цвета"));
    return false;
  }

  ExpressionCompiler compiler(nodes);
  for (size_t index = 0; index < 3U; ++index) {
    if (!compiler.compile(arguments[index], color.channels[index], diagnostics, sourceLine)) {
      return false;
    }
  }

  return true;
}

CompiledSprite compileSprite(const lamp::live::dsl::SpriteDeclaration& sprite) {
  CompiledSprite compiledSprite;
  compiledSprite.name = sprite.name;

  int16_t y = 0;
  int16_t maxWidth = 0;
  std::string currentLine;
  for (char ch : sprite.bitmap) {
    if (ch == '\n') {
      maxWidth = std::max(maxWidth, static_cast<int16_t>(currentLine.size()));
      ++y;
      currentLine.clear();
      continue;
    }

    const int16_t x = static_cast<int16_t>(currentLine.size());
    currentLine.push_back(ch);
    if (ch != '.' && ch != ' ' && ch != '\t') {
      CompiledSpritePixel pixel;
      pixel.x = x;
      pixel.y = y;
      compiledSprite.pixels.push_back(pixel);
    }
  }

  maxWidth = std::max(maxWidth, static_cast<int16_t>(currentLine.size()));
  compiledSprite.width = maxWidth;
  compiledSprite.height = static_cast<int16_t>(y + 1);

  return compiledSprite;
}

}  // namespace

bool Compiler::compile(const dsl::Program& program, CompiledProgram& compiledProgram,
                       std::vector<lamp::live::Diagnostic>& diagnostics) const {
  CompiledProgram compiled;
  compiled.effectName = program.effectName;

  for (const lamp::live::dsl::SpriteDeclaration& sprite : program.sprites) {
    compiled.sprites.push_back(compileSprite(sprite));
  }

  ExpressionCompiler expressionCompiler(compiled.expressions);
  for (const lamp::live::dsl::LayerDeclaration& layer : program.layers) {
    CompiledLayer compiledLayer;
    compiledLayer.name = layer.name;

    size_t spriteIndex = compiled.sprites.size();
    for (size_t index = 0; index < compiled.sprites.size(); ++index) {
      if (compiled.sprites[index].name == layer.spriteName) {
        spriteIndex = index;
        break;
      }
    }
    if (spriteIndex == compiled.sprites.size()) {
      diagnostics.push_back(makeDiagnostic(0, "Не найден sprite: " + layer.spriteName));
      return false;
    }
    compiledLayer.spriteIndex = static_cast<uint16_t>(spriteIndex);

    if (!compileColor(layer.colorExpression, compiled.expressions, compiledLayer.color, diagnostics,
                      layer.colorLine)) {
      return false;
    }
    if (!compileBlendMode(layer.blendMode.empty() ? "normal" : layer.blendMode,
                          compiledLayer.blendMode, diagnostics, layer.blendLine)) {
      return false;
    }
    if (!expressionCompiler.compile(layer.xExpression.empty() ? "0" : layer.xExpression,
                                    compiledLayer.xExpression, diagnostics, layer.xLine) ||
        !expressionCompiler.compile(layer.yExpression.empty() ? "0" : layer.yExpression,
                                    compiledLayer.yExpression, diagnostics, layer.yLine) ||
        !expressionCompiler.compile(layer.scaleExpression.empty() ? "1" : layer.scaleExpression,
                                    compiledLayer.scaleExpression, diagnostics, layer.scaleLine) ||
        !expressionCompiler.compile(layer.rotationExpression.empty() ? "0" : layer.rotationExpression,
                                    compiledLayer.rotationExpression, diagnostics,
                                    layer.rotationLine) ||
        !expressionCompiler.compile(layer.visibleExpression.empty() ? "1" : layer.visibleExpression,
                                    compiledLayer.visibleExpression, diagnostics, layer.visibleLine)) {
      return false;
    }

    compiled.layers.push_back(compiledLayer);
  }

  compiledProgram = compiled;
  return true;
}

}  // namespace lamp::live::runtime