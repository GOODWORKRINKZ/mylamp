#include <unity.h>

#include "FrameBuffer.h"

namespace {

void test_set_pixel_uses_wrapped_x_coordinates() {
  lamp::MatrixLayout layout;
  lamp::FrameBuffer buffer(layout);
  const lamp::Rgb color{12, 34, 56};

  buffer.setPixel(32, 0, color);

  const uint16_t index = layout.toLinearIndex(0, 0);
  const lamp::Rgb physical = buffer.pixelAtIndex(index);
  TEST_ASSERT_EQUAL_UINT8(color.r, physical.r);
  TEST_ASSERT_EQUAL_UINT8(color.g, physical.g);
  TEST_ASSERT_EQUAL_UINT8(color.b, physical.b);
}

void test_clear_resets_all_pixels_to_black() {
  lamp::MatrixLayout layout;
  lamp::FrameBuffer buffer(layout);

  buffer.setPixel(1, 1, lamp::Rgb{1, 2, 3});
  buffer.setPixel(17, 4, lamp::Rgb{4, 5, 6});
  buffer.clear();

  for (uint16_t index = 0; index < buffer.size(); ++index) {
    const lamp::Rgb pixel = buffer.pixelAtIndex(index);
    TEST_ASSERT_EQUAL_UINT8(0, pixel.r);
    TEST_ASSERT_EQUAL_UINT8(0, pixel.g);
    TEST_ASSERT_EQUAL_UINT8(0, pixel.b);
  }
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_set_pixel_uses_wrapped_x_coordinates);
  RUN_TEST(test_clear_resets_all_pixels_to_black);
  return UNITY_END();
}
