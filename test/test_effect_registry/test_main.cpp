#include <unity.h>

#include "FrameBuffer.h"
#include "effects/EffectContext.h"
#include "effects/EffectRegistry.h"
#include "effects/SolidColorEffect.h"

namespace {

void test_registry_switches_active_effect_by_name() {
  lamp::MatrixLayout layout;
  lamp::FrameBuffer buffer(layout);
  lamp::effects::SolidColorEffect first(lamp::Rgb{1, 2, 3}, "solid-color-1");
  lamp::effects::SolidColorEffect second(lamp::Rgb{4, 5, 6}, "solid-color-2");
  lamp::effects::EffectRegistry registry;
  lamp::effects::EffectContext context{0, buffer};

  registry.add(first);
  registry.add(second);

  TEST_ASSERT_TRUE(registry.setActiveByName("solid-color-2"));
  registry.renderActive(context);

  const lamp::Rgb pixel = buffer.getPixel(0, 0);
  TEST_ASSERT_EQUAL_UINT8(4, pixel.r);
  TEST_ASSERT_EQUAL_UINT8(5, pixel.g);
  TEST_ASSERT_EQUAL_UINT8(6, pixel.b);
}

void test_registry_rejects_unknown_effect_names() {
  lamp::effects::EffectRegistry registry;
  lamp::effects::SolidColorEffect first(lamp::Rgb{1, 2, 3}, "solid-color-1");

  registry.add(first);

  TEST_ASSERT_FALSE(registry.setActiveByName("missing"));
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_registry_switches_active_effect_by_name);
  RUN_TEST(test_registry_rejects_unknown_effect_names);
  return UNITY_END();
}
