#include "live/runtime/Executor.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "AppConfig.h"
#include "effects/ClockOverlay.h"

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


// Platform-adaptive hardware random
#ifdef ESP_PLATFORM
#include "esp_system.h"
inline uint32_t hwRandom() { return esp_random(); }
#else
#include <cstdlib>
inline uint32_t hwRandom() { return static_cast<uint32_t>(rand()); }
#endif

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
      return denominator == 0.0f ? 0.0f : evaluateNode(nodes, node.children[0], context, depth + 1, frameRandCache) / denominator;
    }
    case ExpressionOp::kModulo: {
      const float denominator = evaluateNode(nodes, node.children[1], context, depth + 1, frameRandCache);
      return denominator == 0.0f ? 0.0f : std::fmod(evaluateNode(nodes, node.children[0], context, depth + 1, frameRandCache), denominator);
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
    case ExpressionOp::kAnd: {
      const float a = evaluateNode(nodes, node.children[0], context, depth + 1, frameRandCache);
      const float b = evaluateNode(nodes, node.children[1], context, depth + 1, frameRandCache);
      return (a != 0.0f && b != 0.0f) ? 1.0f : 0.0f;
    }
    case ExpressionOp::kNot:
      return evaluateNode(nodes, node.children[0], context, depth + 1, frameRandCache) == 0.0f ? 1.0f : 0.0f;
    case ExpressionOp::kIf: {
      const float cond = evaluateNode(nodes, node.children[0], context, depth + 1, frameRandCache);
      if (cond != 0.0f) {
        return evaluateNode(nodes, node.children[1], context, depth + 1, frameRandCache);
      }
      return evaluateNode(nodes, node.children[2], context, depth + 1, frameRandCache);
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
    case BlendMode::kScreen:
      return lamp::Rgb{
          static_cast<uint8_t>(255U - ((255U - destination.r) * (255U - source.r)) / 255U),
          static_cast<uint8_t>(255U - ((255U - destination.g) * (255U - source.g)) / 255U),
          static_cast<uint8_t>(255U - ((255U - destination.b) * (255U - source.b)) / 255U),
      };
  }

  return source;
}

lamp::Rgb blendWithAlpha(BlendMode blendMode, lamp::Rgb destination, lamp::Rgb source, float alpha) {
  lamp::Rgb blended = blendColors(blendMode, destination, source);
  if (alpha >= 1.0f) return blended;
  if (alpha <= 0.0f) return destination;
  return lamp::Rgb{
      static_cast<uint8_t>(blended.r * alpha + destination.r * (1.0f - alpha)),
      static_cast<uint8_t>(blended.g * alpha + destination.g * (1.0f - alpha)),
      static_cast<uint8_t>(blended.b * alpha + destination.b * (1.0f - alpha)),
  };
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

  const lamp::Rgb destinationColor = frameBuffer.getPixel(physX, physY);
  const lamp::Rgb sourceColor = evaluateColor(program, layer.color, pixelContext, frameRandCache);
  frameBuffer.setPixel(physX, physY, blendColors(layer.blendMode, destinationColor, sourceColor));
}

}  // namespace

void Executor::render(const CompiledProgram& program, const ExecutionContext& context,
                      lamp::FrameBuffer& frameBuffer) const {
  frameBuffer.clear();

  std::vector<float> frameRandCache(program.expressions.size(), NAN);

  EvaluationContext baseContext;
  baseContext.timeSeconds = context.timeSeconds;
  baseContext.deltaSeconds = context.deltaSeconds;
  baseContext.temperatureC = context.temperatureC;
  baseContext.humidityPercent = context.humidityPercent;

  // Phase 7: z-sorting with fixed array (no STL vector heap alloc)
  struct RenderItem {
    float z;
    size_t layerIndex;
    bool isClock;
  };
  RenderItem items[65];  // max 64 layers + 1 clock
  size_t itemCount = 0;

  for (size_t i = 0; i < program.layers.size() && itemCount < 64; ++i) {
    const CompiledLayer& layer = program.layers[i];
    if (layer.spriteIndex >= program.sprites.size()) continue;
    const float visible = evaluateNode(program.expressions, layer.visibleExpression,
                                       baseContext, 0, frameRandCache.data());
    if (visible <= 0.0f) continue;

    float zVal = 0.0f;
    if (layer.zExpression >= 0) {
      zVal = evaluateNode(program.expressions, layer.zExpression, baseContext, 0, frameRandCache.data());
    }
    items[itemCount].z = zVal;
    items[itemCount].layerIndex = i;
    items[itemCount].isClock = false;
    ++itemCount;
  }

  // Add clock overlay to render items
  if (program.clockConfig.enabled && context.clockVisible &&
      context.clockOverlay != nullptr && !context.currentTime.empty() && itemCount < 65) {
    float clockZ = 1.0f;
    if (program.clockConfig.zExpression >= 0) {
      clockZ = evaluateNode(program.expressions, program.clockConfig.zExpression,
                            baseContext, 0, frameRandCache.data());
    }
    items[itemCount].z = clockZ;
    items[itemCount].layerIndex = 0;
    items[itemCount].isClock = true;
    ++itemCount;
  }

  // NOTE: z-sorting sort disabled — causes boot:0xd on ESP32-C3.
  // Items already ordered by declaration; render in that order.
  // TODO: investigate why bubble sort on 65-element struct array crashes.

  // Render in z-order
  for (size_t idx = 0; idx < itemCount; ++idx) {
    const RenderItem& item = items[idx];
    if (item.isClock) {
      float alpha = 1.0f;
      if (program.clockConfig.alphaExpression >= 0) {
        alpha = evaluateNode(program.expressions, program.clockConfig.alphaExpression,
                             baseContext, 0, frameRandCache.data());
        if (alpha < 0.0f) alpha = 0.0f;
        if (alpha > 1.0f) alpha = 1.0f;
      }
      context.clockOverlay->render(context.currentTime, frameBuffer, true,
                                   context.nowMs,
                                   context.temperatureC, context.humidityPercent,
                                   context.sensorAvailable,
                                   program.clockConfig.blendMode, alpha);
      continue;
    }

    const CompiledLayer& layer = program.layers[item.layerIndex];
    const int16_t originX = static_cast<int16_t>(std::lround(
        evaluateNode(program.expressions, layer.xExpression, baseContext, 0, frameRandCache.data())));
    const int16_t originY = static_cast<int16_t>(std::lround(
        evaluateNode(program.expressions, layer.yExpression, baseContext, 0, frameRandCache.data())));
    const int16_t scale = std::max<int16_t>(1, static_cast<int16_t>(std::lround(
        evaluateNode(program.expressions, layer.scaleExpression, baseContext, 0, frameRandCache.data()))));
    const float rotationRadians = evaluateNode(program.expressions, layer.rotationExpression, baseContext, 0, frameRandCache.data());

    const CompiledSprite& sprite = program.sprites[layer.spriteIndex];
    const std::vector<CompiledSpritePixel>* framePixels = &sprite.pixels;
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

    const float centerX = static_cast<float>(originX) + static_cast<float>(sprite.width * scale) * 0.5f;
    const float centerY = static_cast<float>(originY) + static_cast<float>(sprite.height * scale) * 0.5f;
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