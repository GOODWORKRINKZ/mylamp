#include "live/runtime/Executor.h"

#include <algorithm>
#include <cmath>

#include "AppConfig.h"

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

float evaluateNode(const std::vector<ExpressionNode>& nodes, int16_t index,
                   const EvaluationContext& context) {
  if (index < 0 || static_cast<size_t>(index) >= nodes.size()) {
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
      return evaluateNode(nodes, node.children[0], context) +
             evaluateNode(nodes, node.children[1], context);
    case ExpressionOp::kSubtract:
      return evaluateNode(nodes, node.children[0], context) -
             evaluateNode(nodes, node.children[1], context);
    case ExpressionOp::kMultiply:
      return evaluateNode(nodes, node.children[0], context) *
             evaluateNode(nodes, node.children[1], context);
    case ExpressionOp::kDivide: {
      const float denominator = evaluateNode(nodes, node.children[1], context);
      return denominator == 0.0f ? 0.0f : evaluateNode(nodes, node.children[0], context) / denominator;
    }
    case ExpressionOp::kModulo: {
      const float denominator = evaluateNode(nodes, node.children[1], context);
      return denominator == 0.0f ? 0.0f : std::fmod(evaluateNode(nodes, node.children[0], context), denominator);
    }
    case ExpressionOp::kNegate:
      return -evaluateNode(nodes, node.children[0], context);
    case ExpressionOp::kSin:
      return std::sin(evaluateNode(nodes, node.children[0], context));
    case ExpressionOp::kCos:
      return std::cos(evaluateNode(nodes, node.children[0], context));
    case ExpressionOp::kAbs:
      return std::fabs(evaluateNode(nodes, node.children[0], context));
    case ExpressionOp::kMin:
      return std::min(evaluateNode(nodes, node.children[0], context),
                      evaluateNode(nodes, node.children[1], context));
    case ExpressionOp::kMax:
      return std::max(evaluateNode(nodes, node.children[0], context),
                      evaluateNode(nodes, node.children[1], context));
    case ExpressionOp::kClamp:
      return clampFloat(evaluateNode(nodes, node.children[0], context),
                        evaluateNode(nodes, node.children[1], context),
                        evaluateNode(nodes, node.children[2], context));
    case ExpressionOp::kMix: {
      const float a = evaluateNode(nodes, node.children[0], context);
      const float b = evaluateNode(nodes, node.children[1], context);
      const float factor = evaluateNode(nodes, node.children[2], context);
      return a + (b - a) * factor;
    }
    case ExpressionOp::kSmoothstep:
      return smoothstep(evaluateNode(nodes, node.children[0], context),
                        evaluateNode(nodes, node.children[1], context),
                        evaluateNode(nodes, node.children[2], context));
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
                        const EvaluationContext& context) {
  const float first = evaluateNode(program.expressions, color.channels[0], context);
  const float second = evaluateNode(program.expressions, color.channels[1], context);
  const float third = evaluateNode(program.expressions, color.channels[2], context);
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

void renderSpritePixel(const CompiledProgram& program, const CompiledLayer& layer,
                       const EvaluationContext& baseContext, lamp::FrameBuffer& frameBuffer,
                       int16_t renderX, int16_t renderY) {
  EvaluationContext pixelContext = baseContext;
  pixelContext.x = static_cast<float>(renderX);
  pixelContext.y = static_cast<float>(renderY);
  pixelContext.nx = static_cast<float>(renderX) /
                    static_cast<float>(lamp::config::kLogicalWidth - 1U);
  pixelContext.ny = static_cast<float>(renderY) /
                    static_cast<float>(lamp::config::kLogicalHeight - 1U);

  const lamp::Rgb destinationColor = frameBuffer.getPixel(renderX, renderY);
  const lamp::Rgb sourceColor = evaluateColor(program, layer.color, pixelContext);
  frameBuffer.setPixel(renderX, renderY, blendColors(layer.blendMode, destinationColor, sourceColor));
}

}  // namespace

void Executor::render(const CompiledProgram& program, const ExecutionContext& context,
                      lamp::FrameBuffer& frameBuffer) const {
  frameBuffer.clear();

  EvaluationContext baseContext;
  baseContext.timeSeconds = context.timeSeconds;
  baseContext.deltaSeconds = context.deltaSeconds;
  baseContext.temperatureC = context.temperatureC;
  baseContext.humidityPercent = context.humidityPercent;

  for (const CompiledLayer& layer : program.layers) {
    if (layer.spriteIndex >= program.sprites.size()) {
      continue;
    }

    const float visible = evaluateNode(program.expressions, layer.visibleExpression, baseContext);
    if (visible <= 0.0f) {
      continue;
    }

    const int16_t originX = static_cast<int16_t>(std::lround(
        evaluateNode(program.expressions, layer.xExpression, baseContext)));
    const int16_t originY = static_cast<int16_t>(std::lround(
        evaluateNode(program.expressions, layer.yExpression, baseContext)));
    const int16_t scale = std::max<int16_t>(1, static_cast<int16_t>(std::lround(
                                                  evaluateNode(program.expressions, layer.scaleExpression,
                                                               baseContext))));
    const float rotationRadians = evaluateNode(program.expressions, layer.rotationExpression, baseContext);

    const CompiledSprite& sprite = program.sprites[layer.spriteIndex];
    const float centerX = static_cast<float>(originX) +
                          static_cast<float>(sprite.width * scale) * 0.5f;
    const float centerY = static_cast<float>(originY) +
                          static_cast<float>(sprite.height * scale) * 0.5f;
    const float cosRotation = std::cos(rotationRadians);
    const float sinRotation = std::sin(rotationRadians);

    for (const CompiledSpritePixel& pixel : sprite.pixels) {
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

          renderSpritePixel(program, layer, baseContext, frameBuffer, renderX, renderY);
        }
      }
    }
  }
}

}  // namespace lamp::live::runtime