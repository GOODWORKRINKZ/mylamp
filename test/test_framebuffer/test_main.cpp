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

void test_fill_rect_fills_correct_area() {
  lamp::MatrixLayout layout;
  lamp::FrameBuffer buffer(layout);
  const lamp::Rgb red{255, 0, 0};

  buffer.fillRect(2, 3, 5, 2, red);

  for (uint8_t y = 3; y < 5; ++y) {
    for (uint8_t x = 2; x < 7; ++x) {
      const lamp::Rgb p = buffer.getPixel(x, y);
      TEST_ASSERT_EQUAL_UINT8(255, p.r);
      TEST_ASSERT_EQUAL_UINT8(0, p.g);
    }
  }
  // Pixel outside rect should be black.
  const lamp::Rgb outside = buffer.getPixel(1, 3);
  TEST_ASSERT_EQUAL_UINT8(0, outside.r);
}

void test_fill_rect_wraps_across_seam() {
  lamp::MatrixLayout layout;
  lamp::FrameBuffer buffer(layout);
  const lamp::Rgb green{0, 255, 0};

  // Rect spanning x=30..34 → wraps to columns 30,31,0,1,2.
  buffer.fillRect(30, 0, 5, 1, green);

  TEST_ASSERT_EQUAL_UINT8(0, buffer.getPixel(30, 0).r);   // green
  TEST_ASSERT_EQUAL_UINT8(255, buffer.getPixel(30, 0).g);
  TEST_ASSERT_EQUAL_UINT8(0, buffer.getPixel(31, 0).r);   // green
  TEST_ASSERT_EQUAL_UINT8(255, buffer.getPixel(31, 0).g);
  TEST_ASSERT_EQUAL_UINT8(0, buffer.getPixel(0, 0).r);    // green (wrapped!)
  TEST_ASSERT_EQUAL_UINT8(255, buffer.getPixel(0, 0).g);
  TEST_ASSERT_EQUAL_UINT8(0, buffer.getPixel(2, 0).r);    // green
  TEST_ASSERT_EQUAL_UINT8(255, buffer.getPixel(2, 0).g);
  // Pixel at x=3 should be black (outside rect).
  TEST_ASSERT_EQUAL_UINT8(0, buffer.getPixel(3, 0).r);
}

void test_fill_rect_y_out_of_bounds_noop() {
  lamp::MatrixLayout layout;
  lamp::FrameBuffer buffer(layout);
  buffer.fillRect(0, 20, 10, 5, lamp::Rgb{1, 2, 3});  // Y out of range
  // Should not crash; all pixels remain black.
  TEST_ASSERT_EQUAL_UINT8(0, buffer.getPixel(0, 0).r);
}

void test_draw_line_horizontal_wraps() {
  lamp::MatrixLayout layout;
  lamp::FrameBuffer buffer(layout);
  const lamp::Rgb white{255, 255, 255};

  // Line from x=30 to x=2 (wraps across seam).
  buffer.drawLine(30, 5, 2, 5, white);

  TEST_ASSERT_EQUAL_UINT8(255, buffer.getPixel(30, 5).r);
  TEST_ASSERT_EQUAL_UINT8(255, buffer.getPixel(31, 5).r);
  TEST_ASSERT_EQUAL_UINT8(255, buffer.getPixel(0, 5).r);
  TEST_ASSERT_EQUAL_UINT8(255, buffer.getPixel(1, 5).r);
  TEST_ASSERT_EQUAL_UINT8(255, buffer.getPixel(2, 5).r);
}

void test_draw_circle_renders_pixels() {
  lamp::MatrixLayout layout;
  lamp::FrameBuffer buffer(layout);
  const lamp::Rgb blue{0, 0, 255};

  buffer.drawCircle(16, 8, 5, blue);

  // Point at radius distance to the right should be lit.
  const lamp::Rgb edge = buffer.getPixel(21, 8);
  TEST_ASSERT_EQUAL_UINT8(255, edge.b);

  // Point at top of circle should be lit.
  const lamp::Rgb top = buffer.getPixel(16, 3);
  TEST_ASSERT_EQUAL_UINT8(255, top.b);

  // Point outside radius should be black.
  const lamp::Rgb outside = buffer.getPixel(25, 8);
  TEST_ASSERT_EQUAL_UINT8(0, outside.b);
}

void test_draw_circle_wraps_at_seam() {
  lamp::MatrixLayout layout;
  lamp::FrameBuffer buffer(layout);
  const lamp::Rgb red{255, 0, 0};

  // Circle centred at x=0 (the seam).
  buffer.drawCircle(0, 8, 3, red);

  // Should have pixels at both x=0-3 AND x=29-31 (wrapped).
  bool foundLeft = false, foundRight = false;
  for (uint8_t y = 0; y < 16; ++y) {
    if (buffer.getPixel(0, y).r == 255) foundLeft = true;
    if (buffer.getPixel(31, y).r == 255) foundRight = true;
  }
  TEST_ASSERT_TRUE(foundLeft);
  TEST_ASSERT_TRUE(foundRight);
}

void test_angle_to_x_and_back() {
  lamp::MatrixLayout layout;

  TEST_ASSERT_EQUAL_UINT8(0, layout.angleToX(0.0f));
  TEST_ASSERT_EQUAL_UINT8(16, layout.angleToX(180.0f));
  TEST_ASSERT_EQUAL_UINT8(0, layout.angleToX(360.0f));
  TEST_ASSERT_EQUAL_UINT8(8, layout.angleToX(90.0f));
  TEST_ASSERT_EQUAL_UINT8(24, layout.angleToX(270.0f));
  TEST_ASSERT_EQUAL_UINT8(4, layout.angleToX(45.0f));

  // Round-trip.
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, layout.xToAngle(layout.angleToX(0.0f)));
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 180.0f, layout.xToAngle(layout.angleToX(180.0f)));
}

void test_row_and_col_count() {
  lamp::MatrixLayout layout;
  TEST_ASSERT_EQUAL_UINT8(16, layout.rowCount());
  TEST_ASSERT_EQUAL_UINT8(32, layout.colCount());
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_set_pixel_uses_wrapped_x_coordinates);
  RUN_TEST(test_clear_resets_all_pixels_to_black);
  RUN_TEST(test_fill_rect_fills_correct_area);
  RUN_TEST(test_fill_rect_wraps_across_seam);
  RUN_TEST(test_fill_rect_y_out_of_bounds_noop);
  RUN_TEST(test_draw_line_horizontal_wraps);
  RUN_TEST(test_draw_circle_renders_pixels);
  RUN_TEST(test_draw_circle_wraps_at_seam);
  RUN_TEST(test_angle_to_x_and_back);
  RUN_TEST(test_row_and_col_count);
  return UNITY_END();
}
