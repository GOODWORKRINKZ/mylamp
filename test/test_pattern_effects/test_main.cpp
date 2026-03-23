#include <unity.h>

#include "FrameBuffer.h"
#include "effects/AlternatingColumnsEffect.h"
#include "effects/EffectContext.h"

namespace {

void test_alternating_columns_effect_draws_even_and_odd_columns_differently() {
  lamp::MatrixLayout layout;
  lamp::FrameBuffer buffer(layout);
  lamp::effects::AlternatingColumnsEffect effect(lamp::Rgb{10, 0, 0}, lamp::Rgb{0, 10, 0});
  lamp::effects::EffectContext context{0, buffer};

  effect.render(context);

  const lamp::Rgb evenPixel = buffer.getPixel(0, 0);
  const lamp::Rgb oddPixel = buffer.getPixel(1, 0);
  TEST_ASSERT_EQUAL_UINT8(10, evenPixel.r);
  TEST_ASSERT_EQUAL_UINT8(0, evenPixel.g);
  TEST_ASSERT_EQUAL_UINT8(0, oddPixel.r);
  TEST_ASSERT_EQUAL_UINT8(10, oddPixel.g);
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_alternating_columns_effect_draws_even_and_odd_columns_differently);
  return UNITY_END();
}