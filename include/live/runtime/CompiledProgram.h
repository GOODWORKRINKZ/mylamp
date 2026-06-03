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
  kLoopIndex,
  // Phase 6: random functions
  kRandom,       // random(max) — per-pixel, esp_random()
  kRandf,        // randf(max) — per-frame, cached
  // Phase 6: comparison operators (GLSL-style, return 0.0/1.0)
  kGt,           // >
  kLt,           // <
  kGte,          // >=
  kLte,          // <=
  kEq,           // ==
  kNeq,          // !=
  // Phase 6: logical operators
  kAnd,          // &&
  kNot,          // ! (unary)
  // Phase 6: conditional
  kIf,           // if(cond, a, b)
  // Phase 6: compute block reference
  kComputeRef,   // reference to compute block result
};

static constexpr int16_t kMaxUnrolledLayers = 64;

struct ExpressionNode {
  ExpressionOp op = ExpressionOp::kConstant;
  float constant = 0.0f;
  int16_t children[4] = {-1, -1, -1, -1};
};

enum class ColorModel {
  kRgb,
  kHsv,
};

enum class BlendMode {
  kNormal,
  kAdd,
  kMultiply,
};

struct CompiledColor {
  ColorModel model = ColorModel::kRgb;
  int16_t channels[3] = {-1, -1, -1};
};

struct CompiledSpritePixel {
  int16_t x = 0;
  int16_t y = 0;
  uint8_t pr = 0;
  uint8_t pg = 0;
  uint8_t pb = 0;
  bool hasPixelColor = false;
};

struct CompiledSprite {
  std::string name;
  int16_t width = 0;
  int16_t height = 0;
  std::vector<CompiledSpritePixel> pixels;
  std::vector<std::vector<CompiledSpritePixel>> frames;
  bool hasPalette = false;
};

struct CompiledLayer {
  std::string name;
  uint16_t spriteIndex = 0;
  CompiledColor color;
  int16_t xExpression = -1;
  int16_t yExpression = -1;
  int16_t scaleExpression = -1;
  int16_t rotationExpression = -1;
  BlendMode blendMode = BlendMode::kNormal;
  int16_t visibleExpression = -1;
  int16_t frameExpression = -1;
  int16_t zExpression = -1;
};

// Phase 6: Compute block — imperative per-pixel computation
struct CompiledComputeStmt {
  enum Kind { kLet, kAssign, kWhile, kExpr };
  Kind kind = kExpr;
  std::string varName;
  int16_t exprIndex = -1;     // expression index for let/assign/expr
  int16_t condIndex = -1;     // condition index for while
  std::vector<CompiledComputeStmt> body;  // while body
};

struct CompiledComputeBlock {
  std::string name;
  std::vector<CompiledComputeStmt> body;
};

struct CompiledProgram {
  std::string effectName;
  std::vector<CompiledSprite> sprites;
  std::vector<ExpressionNode> expressions;
  std::vector<CompiledLayer> layers;
  std::vector<CompiledComputeBlock> computes;      // Phase 6
  std::vector<std::string> computeNames;            // Phase 6: ordered compute block names
};

}  // namespace lamp::live::runtime