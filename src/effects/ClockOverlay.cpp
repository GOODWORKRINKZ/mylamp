#include "effects/ClockOverlay.h"

#include "AppConfig.h"

namespace lamp::effects {

namespace {

constexpr lamp::Rgb kDigitColor{255, 180, 96};
constexpr lamp::Rgb kSeparatorColor{96, 180, 255};
constexpr int16_t kDigitWidth = 3;   // px per digit
constexpr int16_t kDigitHeight = 5;  // px per digit
constexpr int16_t kDigitGap = 1;     // px gap between chars
constexpr int16_t kOverlayMarginRight = 2;
constexpr int16_t kOverlayMarginTop = 1;

// 3×5 font: digits 0-9.  1 = lit, 0 = dark.
// Row 0 = top, row 4 = bottom.
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

bool isDigit(char value) {
  return value >= '0' && value <= '9';
}

void drawDigit(int16_t originX, int16_t originY, uint8_t digit, lamp::FrameBuffer& fb) {
  if (digit > 9) return;
  for (int16_t row = 0; row < kDigitHeight; ++row) {
    uint8_t bits = kFont[digit][row];
    for (int16_t col = 0; col < kDigitWidth; ++col) {
      if (bits & (0b100 >> col)) {
        fb.setPixel(originX + row, originY + col, kDigitColor);
      }
    }
  }
}

void drawColon(int16_t originX, int16_t originY, lamp::FrameBuffer& fb) {
  fb.setPixel(originX + 1, originY, kSeparatorColor);
  fb.setPixel(originX + 3, originY, kSeparatorColor);
}

}  // namespace

void ClockOverlay::render(const std::string& currentTime, lamp::FrameBuffer& frameBuffer,
                          bool visible) const {
  if (!visible || currentTime.size() < 8U) return;
  if (!isDigit(currentTime[0]) || !isDigit(currentTime[1]) || currentTime[2] != ':' ||
      !isDigit(currentTime[3]) || !isDigit(currentTime[4]) || currentTime[5] != ':' ||
      !isDigit(currentTime[6]) || !isDigit(currentTime[7])) {
    return;
  }

  // HH:MM:SS = 6 digits × 3px + 5 gaps × 1px + 2 colons × 1px = 25 px
  constexpr int16_t kTotal = kDigitWidth * 6 + kDigitGap * 5 + 2;
  const int16_t originY = static_cast<int16_t>(lamp::config::kLogicalHeight) - kTotal - kOverlayMarginRight;
  const int16_t originX = kOverlayMarginTop;

  int16_t y = originY;
  drawDigit(originX, y, currentTime[0] - '0', frameBuffer);  y += kDigitWidth + kDigitGap;
  drawDigit(originX, y, currentTime[1] - '0', frameBuffer);  y += kDigitWidth + kDigitGap;
  drawColon(originX, y, frameBuffer);                         y += 1 + kDigitGap;
  drawDigit(originX, y, currentTime[3] - '0', frameBuffer);  y += kDigitWidth + kDigitGap;
  drawDigit(originX, y, currentTime[4] - '0', frameBuffer);  y += kDigitWidth + kDigitGap;
  drawColon(originX, y, frameBuffer);                         y += 1 + kDigitGap;
  drawDigit(originX, y, currentTime[6] - '0', frameBuffer);  y += kDigitWidth + kDigitGap;
  drawDigit(originX, y, currentTime[7] - '0', frameBuffer);
}

}  // namespace lamp::effects