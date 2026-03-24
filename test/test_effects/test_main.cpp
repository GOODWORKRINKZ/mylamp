#include <unity.h>

#include "FrameBuffer.h"
#include "AppConfig.h"
#include "effects/ClockOverlay.h"
#include "effects/EffectContext.h"
#include "effects/SolidColorEffect.h"

namespace {

void test_solid_color_effect_fills_entire_buffer() {
  lamp::MatrixLayout layout;
  lamp::FrameBuffer buffer(layout);
  lamp::effects::SolidColorEffect effect(lamp::Rgb{8, 16, 32});
  lamp::effects::EffectContext context{1234, buffer};

  effect.render(context);

  for (uint16_t index = 0; index < buffer.size(); ++index) {
    const lamp::Rgb pixel = buffer.pixelAtIndex(index);
    TEST_ASSERT_EQUAL_UINT8(8, pixel.r);
    TEST_ASSERT_EQUAL_UINT8(16, pixel.g);
    TEST_ASSERT_EQUAL_UINT8(32, pixel.b);
  }
}

void test_clock_overlay_renders_binary_columns_for_current_time() {
  lamp::MatrixLayout layout;
  lamp::FrameBuffer buffer(layout);
  lamp::effects::ClockOverlay overlay;

  overlay.render("12:34:56", buffer, true);

  uint16_t litPixels = 0;
  uint16_t separatorPixels = 0;
  for (uint16_t index = 0; index < buffer.size(); ++index) {
    const lamp::Rgb pixel = buffer.pixelAtIndex(index);
    if (pixel.r != 0 || pixel.g != 0 || pixel.b != 0) {
      ++litPixels;
    }

    if (pixel.r == 96 && pixel.g == 180 && pixel.b == 255) {
      ++separatorPixels;
    }
  }

  TEST_ASSERT_GREATER_THAN_UINT16(4U, litPixels);
  TEST_ASSERT_GREATER_THAN_UINT16(0U, separatorPixels);
}

void test_clock_overlay_skips_invalid_time_or_hidden_state() {
  lamp::MatrixLayout layout;
  lamp::FrameBuffer buffer(layout);
  lamp::effects::ClockOverlay overlay;

  overlay.render("bad", buffer, true);
  overlay.render("12:34:56", buffer, false);

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
  RUN_TEST(test_solid_color_effect_fills_entire_buffer);
  RUN_TEST(test_clock_overlay_renders_binary_columns_for_current_time);
  RUN_TEST(test_clock_overlay_skips_invalid_time_or_hidden_state);
  return UNITY_END();
}