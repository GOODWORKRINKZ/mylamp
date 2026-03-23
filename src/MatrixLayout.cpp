#include "MatrixLayout.h"

namespace lamp {

MatrixLayout::MatrixLayout()
    : panels_{{0, true, false, false},
              {config::kPanelWidth, true, false, false}} {}

bool MatrixLayout::isInside(int16_t x, int16_t y) const {
  return x >= 0 && x < config::kLogicalWidth && y >= 0 && y < config::kLogicalHeight;
}

uint16_t MatrixLayout::wrapX(int16_t x) const {
  int16_t wrapped = x % config::kLogicalWidth;
  if (wrapped < 0) {
    wrapped += config::kLogicalWidth;
  }
  return static_cast<uint16_t>(wrapped);
}

uint16_t MatrixLayout::toLinearIndex(int16_t x, int16_t y) const {
  if (y < 0 || y >= config::kLogicalHeight) {
    return kInvalidIndex;
  }

  const uint16_t wrappedX = wrapX(x);
  const uint8_t panelIndex = wrappedX / config::kPanelWidth;
  const PanelLayout& panel = panels_[panelIndex];
  const uint8_t localX = wrappedX - panel.logicalOffsetX;
  const uint8_t localY = static_cast<uint8_t>(y);

  return mapPanelPixel(panelIndex, localX, localY);
}

uint16_t MatrixLayout::mapPanelPixel(uint8_t panelIndex, uint8_t localX, uint8_t localY) const {
  const PanelLayout& panel = panels_[panelIndex];
  uint8_t mappedX = panel.reverseX ? (config::kPanelWidth - 1U) - localX : localX;
  uint8_t mappedY = panel.reverseY ? (config::kPanelHeight - 1U) - localY : localY;

  if (panel.serpentine && (mappedY & 0x01U)) {
    mappedX = (config::kPanelWidth - 1U) - mappedX;
  }

  const uint16_t panelBase = panelIndex * config::kPanelWidth * config::kPanelHeight;
  return panelBase + static_cast<uint16_t>(mappedY) * config::kPanelWidth + mappedX;
}

}  // namespace lamp
