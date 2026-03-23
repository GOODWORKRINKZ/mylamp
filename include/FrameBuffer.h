#pragma once

#include <stdint.h>

#include <vector>

#include "AppConfig.h"
#include "MatrixLayout.h"

namespace lamp {

struct Rgb {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

class FrameBuffer {
 public:
  explicit FrameBuffer(const MatrixLayout& layout);

  void clear();
  void fill(Rgb color);
  void setPixel(int16_t x, int16_t y, Rgb color);
  Rgb getPixel(int16_t x, int16_t y) const;
  Rgb pixelAtIndex(uint16_t index) const;
  uint16_t size() const;

 private:
  const MatrixLayout& layout_;
  std::vector<Rgb> pixels_;
};

}  // namespace lamp
