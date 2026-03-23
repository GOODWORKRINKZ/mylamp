#include "FrameBuffer.h"

namespace lamp {

namespace {

constexpr Rgb kBlack{0, 0, 0};

}  // namespace

FrameBuffer::FrameBuffer(const MatrixLayout& layout)
    : layout_(layout), pixels_(config::kPixelCount, kBlack) {}

void FrameBuffer::clear() {
  fill(kBlack);
}

void FrameBuffer::fill(Rgb color) {
  for (Rgb& pixel : pixels_) {
    pixel = color;
  }
}

void FrameBuffer::setPixel(int16_t x, int16_t y, Rgb color) {
  const uint16_t index = layout_.toLinearIndex(x, y);
  if (index == MatrixLayout::kInvalidIndex || index >= pixels_.size()) {
    return;
  }

  pixels_[index] = color;
}

Rgb FrameBuffer::getPixel(int16_t x, int16_t y) const {
  const uint16_t index = layout_.toLinearIndex(x, y);
  return pixelAtIndex(index);
}

Rgb FrameBuffer::pixelAtIndex(uint16_t index) const {
  if (index >= pixels_.size()) {
    return kBlack;
  }

  return pixels_[index];
}

uint16_t FrameBuffer::size() const {
  return static_cast<uint16_t>(pixels_.size());
}

}  // namespace lamp
