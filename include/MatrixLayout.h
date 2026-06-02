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
  static constexpr float kCircumference = static_cast<float>(config::kLogicalWidth);

  MatrixLayout();

  bool isInside(int16_t x, int16_t y) const;
  uint16_t toLinearIndex(int16_t x, int16_t y) const;
  uint16_t wrapX(int16_t x) const;

  /// Angle → X column: 0° → 0, 360° → 0 (wraps), 180° → 16.
  uint8_t angleToX(float degrees) const;

  /// X column → angle in degrees [0, 360).
  float xToAngle(uint8_t x) const;

  /// Number of rows (Y dimension).
  uint8_t rowCount() const;

  /// Number of columns (X dimension = circumference).
  uint8_t colCount() const;

 private:
  uint16_t mapPanelPixel(uint8_t panelIndex, uint8_t localX, uint8_t localY) const;

  PanelLayout panels_[config::kPanelCount];
};

}  // namespace lamp
