#include "live/runtime/Executor.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>

#include "AppConfig.h"

// Platform-adaptive hardware random
#ifdef ESP_PLATFORM
#include "esp_system.h"
inline uint32_t hwRandom() { return esp_random(); }
#else
#include <cstdlib>
inline uint32_t hwRandom() { return static_cast<uint32_t>(rand()); }
#endif

namespace lamp::live::runtime {

namespace {

struct EvaluationContext {
  float timeSeconds = 0.0f;
  float deltaSeconds = 0.0f;
  float temperatureC = 0.0f;
  float humidityPercent = 0.0f;
  float x = 0.0f;
  float y = 0.0f;
  float nx = 0.0f;
  float ny = 0.0f;
};

float clampFloat(float value, float minValue, float maxValue) {
  return std::max(minValue, std::min(maxValue, value));
}

float smoothstep(float edge0, float edge1, float value) {
  const float normalized = clampFloat((value - edge0) / (edge1 - edge0), 0.0f, 1.0f);
  return normalized * normalized * (3.0f - 2.0f * normalized);
}

// Phase 6: per-pixel compute results pointer
static const float* g_computeResults = nullptr;

float evaluateNode(const std::vector<ExpressionNode>& nodes, int16_t index,
                   const EvaluationContext& context, int16_t depth = 0,
                   float* frameRandCache = nullptr) {
  if (index < 0 || static_cast<size_t>(index) >= nodes.size()) {
    return 0.0f;
  }

  if (depth >= lamp::config::kMaxExpressionDepth) {
    return 0.0f;
  }

  const ExpressionNode& node = nodes[static_cast<size_t>(index)];
  switch (node.op) {
    case ExpressionOp::kConstant:
      return node.constant;
    case ExpressionOp::kTime:
      return context.timeSeconds;
    case ExpressionOp::kDeltaTime:
      return context.deltaSeconds;
    case ExpressionOp::kTemperature:
      return context.temperatureC;
    case ExpressionOp::kHumidity:
      return context.humidityPercent;
    case ExpressionOp::kCoordX:
      return context.x;
    case ExpressionOp::kCoordY:
      return context.y;
    case ExpressionOp::kCoordNx:
      return context.nx;
    case ExpressionOp::kCoordNy:
      return context.ny;
    case ExpressionOp::kAdd:
      return evaluateNode(nodes, node.children[0], context, depth + 1, frameRandCache) +
             evaluateNode(nodes, node.children[1], context, depth + 1, frameRandCache);
    case ExpressionOp::kSubtract:
      return evaluateNode(nodes, node.children[0], context, depth + 1, frameRandCache) -
             evaluateNode(nodes, node.children[1], context, depth + 1, frameRandCache);
    case ExpressionOp::kMultiply:
      return evaluateNode(nodes, node.children[0], context, depth + 1, frameRandCache) *
             evaluateNode(nodes, node.children[1], context, depth + 1, frameRandCache);
    case ExpressionOp::kDivide: {
      const float denominator = evaluateNode(nodes, node.children[1], context, depth + 1, frameRandCache);
      return denominator == 0.0f ? 0.0f :
        evaluateNode(nodes, node.children[0], context, depth + 1, frameRandCache) / denominator;
    }
    case ExpressionOp::kModulo: {
      const float denominator = evaluateNode(nodes, node.children[1], context, depth + 1, frameRandCache);
      return denominator == 0.0f ? 0.0f :
        std::fmod(evaluateNode(nodes, node.children[0], context, depth + 1, frameRandCache), denominator);
    }
    case ExpressionOp::kNegate:
      return -evaluateNode(nodes, node.children[0], context, depth + 1, frameRandCache);
    case ExpressionOp::kSin:
      return std::sin(evaluateNode(nodes, node.children[0], context, depth + 1, frameRandCache));
    case ExpressionOp::kCos:
      return std::cos(evaluateNode(nodes, node.children[0], context, depth + 1, frameRandCache));
    case ExpressionOp::kAbs:
      return std::fabs(evaluateNode(nodes, node.children[0], context, depth + 1, frameRandCache));
    case ExpressionOp::kMin:
      return std::min(evaluateNode(nodes, node.children[0], context, depth + 1, frameRandCache),
                      evaluateNode(nodes, node.children[1], context, depth + 1, frameRandCache));
    case ExpressionOp::kMax:
      return std::max(evaluateNode(nodes, node.children[0], context, depth + 1, frameRandCache),
                      evaluateNode(nodes, node.children[1], context, depth + 1, frameRandCache));
    case ExpressionOp::kClamp:
      return clampFloat(evaluateNode(nodes, node.children[0], context, depth + 1, frameRandCache),
                        evaluateNode(nodes, node.children[1], context, depth + 1, frameRandCache),
                        evaluateNode(nodes, node.children[2], context, depth + 1, frameRandCache));
    case ExpressionOp::kMix: {
      const float a = evaluateNode(nodes, node.children[0], context, depth + 1, frameRandCache);
      const float b = evaluateNode(nodes, node.children[1], context, depth + 1, frameRandCache);
      const float factor = evaluateNode(nodes, node.children[2], context, depth + 1, frameRandCache);
      return a + (b - a) * factor;
    }
    case ExpressionOp::kSmoothstep:
      return smoothstep(evaluateNode(nodes, node.children[0], context, depth + 1, frameRandCache),
                        evaluateNode(nodes, node.children[1], context, depth + 1, frameRandCache),
                        evaluateNode(nodes, node.children[2], context, depth + 1, frameRandCache));
    // Phase 6: random functions
    case ExpressionOp::kRandom: {
      const float maxVal = evaluateNode(nodes, node.children[0], context, depth + 1, frameRandCache);
      if (maxVal <= 0.0f) return 0.0f;
      return (static_cast<float>(hwRandom()) / 4294967295.0f) * maxVal;
    }
    case ExpressionOp::kRandf: {
      if (frameRandCache != nullptr && !std::isnan(frameRandCache[index])) {
        return frameRandCache[index];
      }
      const float maxVal = evaluateNode(nodes, node.children[0], context, depth + 1, frameRandCache);
      float result = 0.0f;
      if (maxVal > 0.0f) {
        result = (static_cast<float>(hwRandom()) / 4294967295.0f) * maxVal;
      }
      if (frameRandCache != nullptr) {
        frameRandCache[index] = result;
      }
      return result;
    }
    // Phase 6: comparison operators
    case ExpressionOp::kGt:
      return evaluateNode(nodes, node.children[0], context, depth + 1, frameRandCache) >
             evaluateNode(nodes, node.children[1], context, depth + 1, frameRandCache) ? 1.0f : 0.0f;
    case ExpressionOp::kLt:
      return evaluateNode(nodes, node.children[0], context, depth + 1, frameRandCache) <
             evaluateNode(nodes, node.children[1], context, depth + 1, frameRandCache) ? 1.0f : 0.0f;
    case ExpressionOp::kGte:
      return evaluateNode(nodes, node.children[0], context, depth + 1, frameRandCache) >=
             evaluateNode(nodes, node.children[1], context, depth + 1, frameRandCache) ? 1.0f : 0.0f;
    case ExpressionOp::kLte:
      return evaluateNode(nodes, node.children[0], context, depth + 1, frameRandCache) <=
             evaluateNode(nodes, node.children[1], context, depth + 1, frameRandCache) ? 1.0f : 0.0f;
    case ExpressionOp::kEq: {
      const float diff = evaluateNode(nodes, node.children[0], context, depth + 1, frameRandCache) -
                         evaluateNode(nodes, node.children[1], context, depth + 1, frameRandCache);
      return std::fabs(diff) < 0.00001f ? 1.0f : 0.0f;
    }
    case ExpressionOp::kNeq: {
      const float diff = evaluateNode(nodes, node.children[0], context, depth + 1, frameRandCache) -
                         evaluateNode(nodes, node.children[1], context, depth + 1, frameRandCache);
      return std::fabs(diff) >= 0.00001f ? 1.0f : 0.0f;
    }
    // Phase 6: logical operators
    case ExpressionOp::kAnd: {
      const float a = evaluateNode(nodes, node.children[0], context, depth + 1, frameRandCache);
      const float b = evaluateNode(nodes, node.children[1], context, depth + 1, frameRandCache);
      return (a != 0.0f && b != 0.0f) ? 1.0f : 0.0f;
    }
    case ExpressionOp::kNot:
      return evaluateNode(nodes, node.children[0], context, depth + 1, frameRandCache) == 0.0f ? 1.0f : 0.0f;
    // Phase 6: conditional
    case ExpressionOp::kIf: {
      const float cond = evaluateNode(nodes, node.children[0], context, depth + 1, frameRandCache);
      if (cond != 0.0f) {
        return evaluateNode(nodes, node.children[1], context, depth + 1, frameRandCache);
      }
      return evaluateNode(nodes, node.children[2], context, depth + 1, frameRandCache);
    }
    // Phase 6: compute block reference
    case ExpressionOp::kComputeRef: {
      int ci = static_cast<int>(node.constant);
      if (g_computeResults != nullptr && ci >= 0) {
        return g_computeResults[ci];
      }
      return 0.0f;
    }
  }

  return 0.0f;
}

lamp::Rgb hsvToRgb(float hueDegrees, float saturation, float value) {
  const float hue = std::fmod(hueDegrees, 360.0f) / 60.0f;
  const float chroma = value * saturation;
  const float x = chroma * (1.0f - std::fabs(std::fmod(hue, 2.0f) - 1.0f));
  float red = 0.0f;
  float green = 0.0f;
  float blue = 0.0f;

  if (hue >= 0.0f && hue < 1.0f) {
    red = chroma;
    green = x;
  } else if (hue < 2.0f) {
    red = x;
    green = chroma;
  } else if (hue < 3.0f) {
    green = chroma;
    blue = x;
  } else if (hue < 4.0f) {
    green = x;
    blue = chroma;
  } else if (hue < 5.0f) {
    red = x;
    blue = chroma;
  } else {
    red = chroma;
    blue = x;
  }

  const float match = value - chroma;
  return lamp::Rgb{static_cast<uint8_t>(clampFloat((red + match) * 255.0f, 0.0f, 255.0f)),
                   static_cast<uint8_t>(clampFloat((green + match) * 255.0f, 0.0f, 255.0f)),
                   static_cast<uint8_t>(clampFloat((blue + match) * 255.0f, 0.0f, 255.0f))};
}

lamp::Rgb evaluateColor(const CompiledProgram& program, const CompiledColor& color,
                        const EvaluationContext& context, float* frameRandCache = nullptr) {
  const float first = evaluateNode(program.expressions, color.channels[0], context, 0, frameRandCache);
  const float second = evaluateNode(program.expressions, color.channels[1], context, 0, frameRandCache);
  const float third = evaluateNode(program.expressions, color.channels[2], context, 0, frameRandCache);
  if (color.model == ColorModel::kHsv) {
    return hsvToRgb(first, second, third);
  }

  return lamp::Rgb{static_cast<uint8_t>(clampFloat(first, 0.0f, 255.0f)),
                   static_cast<uint8_t>(clampFloat(second, 0.0f, 255.0f)),
                   static_cast<uint8_t>(clampFloat(third, 0.0f, 255.0f))};
}

lamp::Rgb blendColors(BlendMode blendMode, lamp::Rgb destination, lamp::Rgb source) {
  switch (blendMode) {
    case BlendMode::kNormal:
      return source;
    case BlendMode::kAdd:
      return lamp::Rgb{
          static_cast<uint8_t>(std::min<uint16_t>(255U, destination.r + source.r)),
          static_cast<uint8_t>(std::min<uint16_t>(255U, destination.g + source.g)),
          static_cast<uint8_t>(std::min<uint16_t>(255U, destination.b + source.b)),
      };
    case BlendMode::kMultiply:
      return lamp::Rgb{
          static_cast<uint8_t>((static_cast<uint16_t>(destination.r) * source.r) / 255U),
          static_cast<uint8_t>((static_cast<uint16_t>(destination.g) * source.g) / 255U),
          static_cast<uint8_t>((static_cast<uint16_t>(destination.b) * source.b) / 255U),
      };
  }

  return source;
}

// Phase 6: execute compute block statements (per-pixel)
void execComputeStmts(const std::vector<CompiledComputeStmt>& stmts,
                      const std::vector<ExpressionNode>& expressions,
                      const EvaluationContext& ctx, float* frameRandCache,
                      std::unordered_map<std::string, float>& vars, float& lastResult) {
  for (const auto& stmt : stmts) {
    if (stmt.kind == CompiledComputeStmt::kLet || stmt.kind == CompiledComputeStmt::kAssign) {
      float val = evaluateNode(expressions, stmt.exprIndex, ctx, 0, frameRandCache);
      vars[stmt.varName] = val;
      lastResult = val;
    } else if (stmt.kind == CompiledComputeStmt::kWhile) {
      int iter = 0;
      while (iter < lamp::config::kMaxExpressionDepth) {
        float cond = evaluateNode(expressions, stmt.condIndex, ctx, 0, frameRandCache);
        if (cond == 0.0f) break;
        execComputeStmts(stmt.body, expressions, ctx, frameRandCache, vars, lastResult);
        ++iter;
      }
    } else if (stmt.kind == CompiledComputeStmt::kExpr) {
      float val = evaluateNode(expressions, stmt.exprIndex, ctx, 0, frameRandCache);
      lastResult = val;
    }
  }
}

void renderSpritePixel(const CompiledProgram& program, const CompiledLayer& layer,
                       const EvaluationContext& baseContext, lamp::FrameBuffer& frameBuffer,
                       int16_t renderX, int16_t renderY, float* frameRandCache = nullptr) {
  // XY swap: DSL "x" axis = physical Y (cylinder circumference, wraps),
  //          DSL "y" axis = physical X (cylinder height, bounded).
  const int16_t physX = renderY;  // DSL x → physical Y
  const int16_t physY = renderX;  // DSL y → physical X

  EvaluationContext pixelContext = baseContext;
  pixelContext.x = static_cast<float>(physX);
  pixelContext.y = static_cast<float>(physY);
  pixelContext.nx = static_cast<float>(physX) /
                    static_cast<float>(lamp::config::kLogicalHeight - 1U);
  pixelContext.ny = static_cast<float>(physY) /
                    static_cast<float>(lamp::config::kLogicalWidth - 1U);

  // Phase 6: evaluate compute blocks per-pixel
  std::vector<float> computeResults(program.computes.size(), 0.0f);
  for (size_t ci = 0; ci < program.computes.size(); ++ci) {
    std::unordered_map<std::string, float> vars;
    vars["x"] = pixelContext.x;
    vars["y"] = pixelContext.y;
    vars["nx"] = pixelContext.nx;
    vars["ny"] = pixelContext.ny;
    vars["t"] = pixelContext.timeSeconds;
    vars["dt"] = pixelContext.deltaSeconds;
    float result = 0.0f;
    execComputeStmts(program.computes[ci].body, program.expressions,
                     pixelContext, frameRandCache, vars, result);
    computeResults[ci] = result;
  }
  g_computeResults = computeResults.data();

  const lamp::Rgb destinationColor = frameBuffer.getPixel(physX, physY);
  const lamp::Rgb sourceColor = evaluateColor(program, layer.color, pixelContext, frameRandCache);
  frameBuffer.setPixel(physX, physY, blendColors(layer.blendMode, destinationColor, sourceColor));
}

}  // namespace

void Executor::render(const CompiledProgram& program, const ExecutionContext& context,
                      lamp::FrameBuffer& frameBuffer) const {
  frameBuffer.clear();

  lastRenderOnTop_ = false;

  // Phase 6: per-frame random cache for randf()
  std::vector<float> frameRandCache(program.expressions.size(), NAN);

  EvaluationContext baseContext;
  baseContext.timeSeconds = context.timeSeconds;
  baseContext.deltaSeconds = context.deltaSeconds;
  baseContext.temperatureC = context.temperatureC;
  baseContext.humidityPercent = context.humidityPercent;

  for (const CompiledLayer& layer : program.layers) {
    if (layer.spriteIndex >= program.sprites.size()) {
      continue;
    }

    const float visible = evaluateNode(program.expressions, layer.visibleExpression, baseContext, 0, frameRandCache.data());
    if (visible <= 0.0f) {
      continue;
    }

    // Check z-index: if any visible layer has z >= 1, effect renders on top of clock
    if (layer.zExpression >= 0) {
      float zVal = evaluateNode(program.expressions, layer.zExpression, baseContext, 0, frameRandCache.data());
      if (zVal >= 1.0f) {
        lastRenderOnTop_ = true;
      }
    }

    const int16_t originX = static_cast<int16_t>(std::lround(
        evaluateNode(program.expressions, layer.xExpression, baseContext, 0, frameRandCache.data())));
    const int16_t originY = static_cast<int16_t>(std::lround(
        evaluateNode(program.expressions, layer.yExpression, baseContext, 0, frameRandCache.data())));
    const int16_t scale = std::max<int16_t>(1, static_cast<int16_t>(std::lround(
                                                  evaluateNode(program.expressions, layer.scaleExpression,
                                                               baseContext, 0, frameRandCache.data()))));
    const float rotationRadians = evaluateNode(program.expressions, layer.rotationExpression, baseContext, 0, frameRandCache.data());

    const CompiledSprite& sprite = program.sprites[layer.spriteIndex];

    // Determine which frame's pixels to render
    const std::vector<CompiledSpritePixel>* framePixels = &sprite.pixels;  // default: single-frame
    if (!sprite.frames.empty()) {
      int frameIndex = 0;
      if (layer.frameExpression >= 0) {
        float rawIndex = evaluateNode(program.expressions, layer.frameExpression, baseContext, 0, frameRandCache.data());
        int numFrames = static_cast<int>(sprite.frames.size());
        frameIndex = static_cast<int>(std::floor(rawIndex)) % numFrames;
        if (frameIndex < 0) frameIndex += numFrames;
      }
      framePixels = &sprite.frames[static_cast<size_t>(frameIndex)];
    }

    const float centerX = static_cast<float>(originX) +
                          static_cast<float>(sprite.width * scale) * 0.5f;
    const float centerY = static_cast<float>(originY) +
                          static_cast<float>(sprite.height * scale) * 0.5f;
    const float cosRotation = std::cos(rotationRadians);
    const float sinRotation = std::sin(rotationRadians);

    for (const CompiledSpritePixel& pixel : *framePixels) {
      for (int16_t scaledY = 0; scaledY < scale; ++scaledY) {
        for (int16_t scaledX = 0; scaledX < scale; ++scaledX) {
          const float pixelCenterX = static_cast<float>(originX + pixel.x * scale + scaledX) + 0.5f;
          const float pixelCenterY = static_cast<float>(originY + pixel.y * scale + scaledY) + 0.5f;
          const float offsetX = pixelCenterX - centerX;
          const float offsetY = pixelCenterY - centerY;
          const int16_t renderX = static_cast<int16_t>(std::lround(
              centerX + offsetX * cosRotation - offsetY * sinRotation - 0.5f));
          const int16_t renderY = static_cast<int16_t>(std::lround(
              centerY + offsetX * sinRotation + offsetY * cosRotation - 0.5f));

          if (pixel.hasPixelColor) {
            // Direct per-pixel palette color (bypasses layer color expression)
            const int16_t physX = renderY;
            const int16_t physY = renderX;
            const lamp::Rgb destinationColor = frameBuffer.getPixel(physX, physY);
            const lamp::Rgb sourceColor{pixel.pr, pixel.pg, pixel.pb};
            frameBuffer.setPixel(physX, physY,
                                 blendColors(layer.blendMode, destinationColor, sourceColor));
          } else {
            renderSpritePixel(program, layer, baseContext, frameBuffer, renderX, renderY, frameRandCache.data());
          }
        }
      }
    }
  }
}

}  // namespace lamp::live::runtime