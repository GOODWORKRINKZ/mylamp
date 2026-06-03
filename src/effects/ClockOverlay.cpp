#include "effects/ClockOverlay.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>

#include "AppConfig.h"

namespace lamp::effects {

namespace {

using lamp::live::runtime::BlendMode;

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

lamp::Rgb blendWithAlpha(BlendMode mode, lamp::Rgb dst, lamp::Rgb src, float alpha) {
  lamp::Rgb blended = blendColors(mode, dst, src);
  if (alpha >= 1.0f) return blended;
  if (alpha <= 0.0f) return dst;
  return lamp::Rgb{
      static_cast<uint8_t>(blended.r * alpha + dst.r * (1.0f - alpha)),
      static_cast<uint8_t>(blended.g * alpha + dst.g * (1.0f - alpha)),
      static_cast<uint8_t>(blended.b * alpha + dst.b * (1.0f - alpha)),
  };
}

constexpr lamp::Rgb kDigitColor{255, 220, 160};
constexpr lamp::Rgb kSeparatorColor{180, 220, 255};
constexpr int16_t kDigitWidth = 3;   // px per digit
constexpr int16_t kDigitHeight = 5;  // px per digit
constexpr int16_t kDigitGap = 1;     // px gap between chars
constexpr int16_t kOverlayMarginRight = 2;
constexpr int16_t kOverlayMarginTop = 1;

// 3×5 font: digits + extra glyphs.  1 = lit, 0 = dark.
static const uint8_t kFont[10][5] = {
  {0b111, 0b101, 0b101, 0b101, 0b111},  // 0
  {0b010, 0b110, 0b010, 0b010, 0b111},  // 1
  {0b111, 0b001, 0b111, 0b100, 0b111},  // 2
  {0b111, 0b001, 0b111, 0b001, 0b111},  // 3
  {0b101, 0b101, 0b111, 0b001, 0b001},  // 4
  {0b111, 0b100, 0b111, 0b001, 0b111},  // 5
  {0b111, 0b100, 0b111, 0b101, 0b111},  // 6
  {0b111, 0b001, 0b010, 0b010, 0b010},  // 7
  {0b111, 0b101, 0b111, 0b101, 0b111},  // 8
  {0b111, 0b101, 0b111, 0b001, 0b111},  // 9
};

// Extra 3×5 glyphs: index = char - '0', offset by 10 for non-digits.
static const uint8_t kGlyphC[5] = {0b111, 0b100, 0b100, 0b100, 0b111};   // 'C'
static const uint8_t kGlyphPct[5] = {0b101, 0b001, 0b010, 0b100, 0b101}; // '%'
static const uint8_t kGlyphDeg[5] = {0b010, 0b101, 0b010, 0b000, 0b000}; // '°'

const uint8_t* glyphFor(char c) {
  if (c >= '0' && c <= '9') return kFont[c - '0'];
  if (c == 'C' || c == 'c') return kGlyphC;
  if (c == '%') return kGlyphPct;
  if (c == '\xB0') return kGlyphDeg;  // °
  return nullptr;
}

void drawChar(int16_t originX, int16_t originY, char c, lamp::Rgb color, lamp::FrameBuffer& fb,
              BlendMode blendMode = BlendMode::kNormal, float alpha = 1.0f) {
  const uint8_t* g = glyphFor(c);
  if (!g) return;
  for (int16_t row = 0; row < kDigitHeight; ++row) {
    for (int16_t col = 0; col < kDigitWidth; ++col) {
      if (g[row] & (1 << (kDigitWidth - 1 - col))) {
        const int16_t px = originX + col;
        const int16_t py = originY + row;
        const lamp::Rgb dst = fb.getPixel(px, py);
        fb.setPixel(px, py, blendWithAlpha(blendMode, dst, color, alpha));
      }
    }
  }
}

void drawString(int16_t x, int16_t y, const char* text, lamp::Rgb color, lamp::FrameBuffer& fb,
                BlendMode blendMode = BlendMode::kNormal, float alpha = 1.0f) {
  int16_t cx = x;
  for (size_t i = 0; text[i] != '\0'; ++i) {
    drawChar(cx, y, text[i], color, fb, blendMode, alpha);
    cx += kDigitWidth + kDigitGap;
  }
}

void drawNumber2(int16_t x, int16_t y, int16_t num, lamp::Rgb color, lamp::FrameBuffer& fb,
                 BlendMode blendMode = BlendMode::kNormal, float alpha = 1.0f) {
  char buf[4];
  snprintf(buf, sizeof(buf), "%d", num);
  drawString(x, y, buf, color, fb, blendMode, alpha);
}

void drawColon(int16_t originX, int16_t originY, lamp::FrameBuffer& fb,
               BlendMode blendMode = BlendMode::kNormal, float alpha = 1.0f) {
  const lamp::Rgb d1 = fb.getPixel(originX + 1, originY);
  const lamp::Rgb d3 = fb.getPixel(originX + 3, originY);
  fb.setPixel(originX + 1, originY, blendWithAlpha(blendMode, d1, kSeparatorColor, alpha));
  fb.setPixel(originX + 3, originY, blendWithAlpha(blendMode, d3, kSeparatorColor, alpha));
}

}  // namespace

void ClockOverlay::render(const std::string& currentTime, lamp::FrameBuffer& frameBuffer,
                          bool visible, uint32_t nowMs,
                          float temperatureC, float humidityPercent, bool sensorAvailable,
                          BlendMode blendMode, float alpha) const {
  if (!visible || currentTime.size() < 8U) return;
  if (!std::isdigit(static_cast<unsigned char>(currentTime[0])) ||
      !std::isdigit(static_cast<unsigned char>(currentTime[1])) || currentTime[2] != ':' ||
      !std::isdigit(static_cast<unsigned char>(currentTime[3])) ||
      !std::isdigit(static_cast<unsigned char>(currentTime[4])) || currentTime[5] != ':' ||
      !std::isdigit(static_cast<unsigned char>(currentTime[6])) ||
      !std::isdigit(static_cast<unsigned char>(currentTime[7]))) {
    return;
  }

  constexpr int16_t kTotal = kDigitWidth * 6 + kDigitGap * 5 + 2;

  // Smooth rotation: 3 rpm = 1 revolution per 20 s.  Step 1 px every 625 ms.
  constexpr uint32_t kRotationPeriodMs = 20000;
  const int16_t yOffset = static_cast<int16_t>(
      (nowMs % kRotationPeriodMs) * static_cast<uint32_t>(lamp::config::kLogicalHeight)
      / kRotationPeriodMs);

  const int16_t originY = static_cast<int16_t>(lamp::config::kLogicalHeight)
                          - kTotal - kOverlayMarginRight - yOffset;
  const int16_t originX = kOverlayMarginTop;

  int16_t y = originY;
  drawChar(originX, y, currentTime[0], kDigitColor, frameBuffer, blendMode, alpha);  y += kDigitWidth + kDigitGap;
  drawChar(originX, y, currentTime[1], kDigitColor, frameBuffer, blendMode, alpha);  y += kDigitWidth + kDigitGap;
  drawColon(originX, y, frameBuffer, blendMode, alpha);                               y += 1 + kDigitGap;
  drawChar(originX, y, currentTime[3], kDigitColor, frameBuffer, blendMode, alpha);  y += kDigitWidth + kDigitGap;
  drawChar(originX, y, currentTime[4], kDigitColor, frameBuffer, blendMode, alpha);  y += kDigitWidth + kDigitGap;
  drawColon(originX, y, frameBuffer, blendMode, alpha);                               y += 1 + kDigitGap;
  drawChar(originX, y, currentTime[6], kDigitColor, frameBuffer, blendMode, alpha);  y += kDigitWidth + kDigitGap;
  drawChar(originX, y, currentTime[7], kDigitColor, frameBuffer, blendMode, alpha);

  // Sensor line below clock — rotates opposite direction.
  if (sensorAvailable) {
    constexpr lamp::Rgb kTempColor{200, 240, 255};
    constexpr lamp::Rgb kHumColor{180, 255, 200};
    const int16_t sx = originX + kDigitHeight + 2;
    const int16_t sensorY = static_cast<int16_t>(lamp::config::kLogicalHeight)
                             - kTotal - kOverlayMarginRight + yOffset;

    char buf[6];
    snprintf(buf, sizeof(buf), "%d", static_cast<int>(temperatureC));
    drawString(sx, sensorY, buf, kTempColor, frameBuffer, blendMode, alpha);
    int16_t sy = sensorY + static_cast<int16_t>(strlen(buf)) * (kDigitWidth + kDigitGap);
    drawChar(sx, sy, 'C', kTempColor, frameBuffer, blendMode, alpha);
    sy += kDigitWidth + kDigitGap + 2;
    snprintf(buf, sizeof(buf), "%d", static_cast<int>(humidityPercent));
    drawString(sx, sy, buf, kHumColor, frameBuffer, blendMode, alpha);
    sy += static_cast<int16_t>(strlen(buf)) * (kDigitWidth + kDigitGap);
    drawChar(sx, sy, '%', kHumColor, frameBuffer, blendMode, alpha);
  }
}

}  // namespace lamp::effects