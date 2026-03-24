#pragma once

#include <stdint.h>

#include <string>
#include <vector>

namespace lamp::live::runtime {

enum class ExpressionOp {
  kConstant,
  kTime,
  kDeltaTime,
  kTemperature,
  kHumidity,
  kCoordX,
  kCoordY,
  kCoordNx,
  kCoordNy,
  kAdd,
  kSubtract,
  kMultiply,
  kDivide,
  kModulo,
  kNegate,
  kSin,
  kCos,
  kAbs,
  kMin,
  kMax,
  kClamp,
  kMix,
  kSmoothstep,
};

struct ExpressionNode {
  ExpressionOp op = ExpressionOp::kConstant;
  float constant = 0.0f;
  int16_t children[4] = {-1, -1, -1, -1};
};

enum class ColorModel {
  kRgb,
  kHsv,
};

struct CompiledColor {
  ColorModel model = ColorModel::kRgb;
  int16_t channels[3] = {-1, -1, -1};
};

struct CompiledSpritePixel {
  int16_t x = 0;
  int16_t y = 0;
};

struct CompiledSprite {
  std::string name;
  int16_t width = 0;
  int16_t height = 0;
  std::vector<CompiledSpritePixel> pixels;
};

struct CompiledLayer {
  std::string name;
  uint16_t spriteIndex = 0;
  CompiledColor color;
  int16_t xExpression = -1;
  int16_t yExpression = -1;
  int16_t scaleExpression = -1;
  int16_t rotationExpression = -1;
  int16_t visibleExpression = -1;
};

struct CompiledProgram {
  std::string effectName;
  std::vector<CompiledSprite> sprites;
  std::vector<ExpressionNode> expressions;
  std::vector<CompiledLayer> layers;
};

}  // namespace lamp::live::runtime