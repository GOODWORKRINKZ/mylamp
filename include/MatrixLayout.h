#pragma once

#include <stdint.h>

#include "AppConfig.h"

namespace lamp {

struct PanelLayout {
  uint8_t logicalOffsetX;
  bool serpentine;
  bool reverseX;
  bool reverseY;
};

class MatrixLayout {
 public:
  static constexpr uint16_t kInvalidIndex = 0xFFFF;

  MatrixLayout();

  bool isInside(int16_t x, int16_t y) const;
  uint16_t toLinearIndex(int16_t x, int16_t y) const;
  uint16_t wrapX(int16_t x) const;

 private:
  uint16_t mapPanelPixel(uint8_t panelIndex, uint8_t localX, uint8_t localY) const;

  PanelLayout panels_[config::kPanelCount];
};

}  // namespace lamp
