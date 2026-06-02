#include "MatrixLayout.h"

#include <cmath>

namespace lamp {

// Panels stacked along Y axis (cylinder circumference).
// Panel 0: Y=0..15, Panel 1: Y=16..31.
// Both panels share X=0..15 (cylinder height).
MatrixLayout::MatrixLayout()
    : panels_{{0, true, false, false},                      // Panel 0: normal
              {config::kPanelHeight, true, false, false}} {}  // Panel 1: no reversal

bool MatrixLayout::isInside(int16_t x, int16_t y) const {
  return x >= 0 && x < config::kLogicalWidth && y >= 0 && y < config::kLogicalHeight;
}

uint16_t MatrixLayout::wrapX(int16_t x) const {
  if (x < 0) return kInvalidIndex;
  if (x >= config::kLogicalWidth) return kInvalidIndex;
  return static_cast<uint16_t>(x);
}

uint16_t MatrixLayout::wrapY(int16_t y) const {
  int16_t wrapped = y % config::kLogicalHeight;
  if (wrapped < 0) wrapped += config::kLogicalHeight;
  return static_cast<uint16_t>(wrapped);
}

uint16_t MatrixLayout::toLinearIndex(int16_t x, int16_t y) const {
  if (x < 0 || x >= config::kLogicalWidth) return kInvalidIndex;
  const uint16_t localX = static_cast<uint16_t>(x);

  const uint16_t wrappedY = wrapY(y);
  const uint8_t panelIndex = wrappedY / config::kPanelHeight;
  const PanelLayout& panel = panels_[panelIndex];
  const uint8_t localY = wrappedY - panel.logicalOffsetY;

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

uint8_t MatrixLayout::angleToY(float degrees) const {
  float wrapped = fmodf(degrees, 360.0f);
  if (wrapped < 0.0f) wrapped += 360.0f;
  return static_cast<uint8_t>(wrapped / 360.0f * static_cast<float>(config::kLogicalHeight))
         % config::kLogicalHeight;
}

float MatrixLayout::yToAngle(uint8_t y) const {
  return static_cast<float>(y % config::kLogicalHeight)
         / static_cast<float>(config::kLogicalHeight) * 360.0f;
}

uint8_t MatrixLayout::rowCount() const { return config::kLogicalWidth; }

uint8_t MatrixLayout::colCount() const { return config::kLogicalHeight; }

}  // namespace lamp
