#pragma once

#include <stdint.h>

#include "AppConfig.h"

namespace lamp {

struct PanelLayout {
  uint8_t logicalOffsetY;  // Panel's Y offset in logical space
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
  uint16_t wrapY(int16_t y) const;

  /// Angle → Y column: 0° → 0, 360° → 0 (wraps), 180° → 16.
  uint8_t angleToY(float degrees) const;

  /// Y column → angle in degrees [0, 360).
  float yToAngle(uint8_t y) const;

  /// Height of cylinder (X axis).
  uint8_t rowCount() const;

  /// Circumference of cylinder (Y axis).
  uint8_t colCount() const;

 private:
  uint16_t mapPanelPixel(uint8_t panelIndex, uint8_t localX, uint8_t localY) const;

  PanelLayout panels_[config::kPanelCount];
};

}  // namespace lamp
