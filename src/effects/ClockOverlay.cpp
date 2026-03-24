#include "effects/ClockOverlay.h"

#include "AppConfig.h"

namespace lamp::effects {

namespace {

constexpr lamp::Rgb kDigitColor{255, 180, 96};
constexpr lamp::Rgb kSeparatorColor{96, 180, 255};
constexpr int16_t kOverlayHeight = 4;
constexpr int16_t kOverlayWidth = 6;
constexpr int16_t kOverlayMarginRight = 1;
constexpr int16_t kOverlayMarginTop = 1;

bool isDigit(char value) {
  return value >= '0' && value <= '9';
}

}  // namespace

void ClockOverlay::render(const std::string& currentTime, lamp::FrameBuffer& frameBuffer,
                          bool visible) const {
  if (!visible || currentTime.size() < 5U || !isDigit(currentTime[0]) || !isDigit(currentTime[1]) ||
      currentTime[2] != ':' || !isDigit(currentTime[3]) || !isDigit(currentTime[4])) {
    return;
  }

  const int16_t originX = static_cast<int16_t>(lamp::config::kLogicalWidth) -
                          kOverlayWidth - kOverlayMarginRight;
  const int16_t originY = kOverlayMarginTop;

  renderDigitColumn(originX + 0, originY, static_cast<uint8_t>(currentTime[0] - '0'), frameBuffer);
  renderDigitColumn(originX + 1, originY, static_cast<uint8_t>(currentTime[1] - '0'), frameBuffer);
  frameBuffer.setPixel(originX + 2, originY + 1, kSeparatorColor);
  frameBuffer.setPixel(originX + 2, originY + 2, kSeparatorColor);
  renderDigitColumn(originX + 4, originY, static_cast<uint8_t>(currentTime[3] - '0'), frameBuffer);
  renderDigitColumn(originX + 5, originY, static_cast<uint8_t>(currentTime[4] - '0'), frameBuffer);
}

void ClockOverlay::renderDigitColumn(int16_t x, int16_t y, uint8_t digit,
                                     lamp::FrameBuffer& frameBuffer) const {
  for (int16_t bit = 0; bit < kOverlayHeight; ++bit) {
    if (((digit >> bit) & 0x01U) == 0U) {
      continue;
    }

    frameBuffer.setPixel(x, y + (kOverlayHeight - 1 - bit), kDigitColor);
  }
}

}  // namespace lamp::effects