#include "FrameBuffer.h"

#include <cstdlib>

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

void FrameBuffer::fillRect(int16_t x, int16_t y, uint8_t w, uint8_t h, Rgb color) {
  for (uint8_t dy = 0; dy < h; ++dy) {
    for (uint8_t dx = 0; dx < w; ++dx) {
      setPixel(x + dx, y + dy, color);
    }
  }
}

void FrameBuffer::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, Rgb color) {
  // On a cylinder, choose the shorter direction around circumference (Y axis).
  const int16_t h = static_cast<int16_t>(layout_.colCount());  // circumference
  int16_t rawDy = y1 - y0;
  int16_t wrapDy = rawDy;
  if (rawDy > h / 2)       wrapDy = rawDy - h;
  else if (rawDy < -h / 2) wrapDy = rawDy + h;

  int16_t dx = abs(x1 - x0);
  int16_t dy = -abs(wrapDy);
  int16_t sx = x0 < x1 ? 1 : -1;
  int16_t sy = wrapDy >= 0 ? 1 : -1;
  int16_t err = dx + dy;

  while (true) {
    setPixel(x0, y0, color);
    if (x0 == x1 && y0 == y1) break;
    int16_t e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}

void FrameBuffer::drawCircle(int16_t cx, int16_t cy, uint8_t r, Rgb color) {
  // Midpoint circle algorithm.
  int16_t f = 1 - r;
  int16_t dx = 0;
  int16_t dy = r;

  while (dx <= dy) {
    setPixel(cx + dx, cy + dy, color);
    setPixel(cx + dy, cy + dx, color);
    setPixel(cx - dx, cy + dy, color);
    setPixel(cx - dy, cy + dx, color);
    setPixel(cx + dx, cy - dy, color);
    setPixel(cx + dy, cy - dx, color);
    setPixel(cx - dx, cy - dy, color);
    setPixel(cx - dy, cy - dx, color);
    if (f >= 0) { --dy; f -= 2 * dy; }
    ++dx;
    f += 2 * dx;
  }
}

}  // namespace lamp
