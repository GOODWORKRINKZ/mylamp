#include <unity.h>

#include "FrameBuffer.h"
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

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_solid_color_effect_fills_entire_buffer);
  return UNITY_END();
}