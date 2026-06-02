#include "effects/ClockOverlay.h"

#include <cstdio>
#include <cstring>

#include "AppConfig.h"

namespace lamp::effects {

namespace {

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

void drawChar(int16_t originX, int16_t originY, char c, lamp::Rgb color, lamp::FrameBuffer& fb) {
  const uint8_t* g = glyphFor(c);
  if (!g) return;
  // Dark shadow/outline behind the glyph for contrast.
  for (int16_t row = 0; row < kDigitHeight; ++row) {
    uint8_t bits = g[row];
    for (int16_t col = 0; col < kDigitWidth; ++col) {
      if (bits & (0b100 >> col)) {
        fb.setPixel(originX + row, originY + col, color);
      }
    }
  }
}

void drawDarkBg(int16_t x, int16_t y, int16_t w, int16_t h, lamp::FrameBuffer& fb) {
  constexpr lamp::Rgb kShadow{2, 1, 0};
  for (int16_t dy = 0; dy < h; ++dy)
    for (int16_t dx = 0; dx < w; ++dx)
      fb.setPixel(x + dy, y + dx, kShadow);
}

void drawString(int16_t x, int16_t y, const char* s, lamp::Rgb color, lamp::FrameBuffer& fb) {
  while (*s) {
    drawChar(x, y, *s, color, fb);
    y += kDigitWidth + kDigitGap;
    ++s;
  }
}

void drawNumber2(int16_t x, int16_t y, int16_t num, lamp::Rgb color, lamp::FrameBuffer& fb) {
  char buf[4];
  snprintf(buf, sizeof(buf), "%d", num);
  drawString(x, y, buf, color, fb);
}

bool isDigit(char value) {
  return value >= '0' && value <= '9';
}

void drawColon(int16_t originX, int16_t originY, lamp::FrameBuffer& fb) {
  fb.setPixel(originX + 1, originY, kSeparatorColor);
  fb.setPixel(originX + 3, originY, kSeparatorColor);
}

}  // namespace

void ClockOverlay::render(const std::string& currentTime, lamp::FrameBuffer& frameBuffer,
                          bool visible, uint32_t nowMs,
                          float temperatureC, float humidityPercent, bool sensorAvailable) const {
  if (!visible || currentTime.size() < 8U) return;
  if (!isDigit(currentTime[0]) || !isDigit(currentTime[1]) || currentTime[2] != ':' ||
      !isDigit(currentTime[3]) || !isDigit(currentTime[4]) || currentTime[5] != ':' ||
      !isDigit(currentTime[6]) || !isDigit(currentTime[7])) {
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
  drawChar(originX, y, currentTime[0], kDigitColor, frameBuffer);  y += kDigitWidth + kDigitGap;
  drawChar(originX, y, currentTime[1], kDigitColor, frameBuffer);  y += kDigitWidth + kDigitGap;
  drawColon(originX, y, frameBuffer);                               y += 1 + kDigitGap;
  drawChar(originX, y, currentTime[3], kDigitColor, frameBuffer);  y += kDigitWidth + kDigitGap;
  drawChar(originX, y, currentTime[4], kDigitColor, frameBuffer);  y += kDigitWidth + kDigitGap;
  drawColon(originX, y, frameBuffer);                               y += 1 + kDigitGap;
  drawChar(originX, y, currentTime[6], kDigitColor, frameBuffer);  y += kDigitWidth + kDigitGap;
  drawChar(originX, y, currentTime[7], kDigitColor, frameBuffer);

  // Sensor line below clock — rotates opposite direction.
  if (sensorAvailable) {
    constexpr lamp::Rgb kTempColor{200, 240, 255};
    constexpr lamp::Rgb kHumColor{180, 255, 200};
    const int16_t sx = originX + kDigitHeight + 2;
    const int16_t sensorY = static_cast<int16_t>(lamp::config::kLogicalHeight)
                             - kTotal - kOverlayMarginRight + yOffset;

    char buf[6];
    snprintf(buf, sizeof(buf), "%d", static_cast<int>(temperatureC));
    drawString(sx, sensorY, buf, kTempColor, frameBuffer);
    int16_t sy = sensorY + static_cast<int16_t>(strlen(buf)) * (kDigitWidth + kDigitGap);
    drawChar(sx, sy, 'C', kTempColor, frameBuffer);
    sy += kDigitWidth + kDigitGap + 2;
    snprintf(buf, sizeof(buf), "%d", static_cast<int>(humidityPercent));
    drawString(sx, sy, buf, kHumColor, frameBuffer);
    sy += static_cast<int16_t>(strlen(buf)) * (kDigitWidth + kDigitGap);
    drawChar(sx, sy, '%', kHumColor, frameBuffer);
  }
}

}  // namespace lamp::effects