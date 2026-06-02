#include <unity.h>

#include "FrameBuffer.h"
#include "effects/EffectContext.h"
#include "effects/EffectRegistry.h"
#include "effects/SolidColorEffect.h"

namespace {

// Minimal effect that only fills a single pixel — used to verify that
// the framebuffer is cleared when switching effects.
class SinglePixelEffect : public lamp::effects::IEffect {
 public:
  SinglePixelEffect(int16_t px, int16_t py, lamp::Rgb color, const char* effectName)
      : px_(px), py_(py), color_(color), name_(effectName) {}

  const char* name() const override { return name_; }

  void render(lamp::effects::EffectContext& context) override {
    context.frameBuffer.setPixel(px_, py_, color_);
  }

 private:
  int16_t px_;
  int16_t py_;
  lamp::Rgb color_;
  const char* name_;
};

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

void test_registry_clears_framebuffer_on_effect_switch() {
  lamp::MatrixLayout layout;
  lamp::FrameBuffer buffer(layout);
  SinglePixelEffect effectA(0, 0, lamp::Rgb{255, 0, 0}, "pixel-red");
  SinglePixelEffect effectB(1, 1, lamp::Rgb{0, 255, 0}, "pixel-green");
  lamp::effects::EffectRegistry registry;
  lamp::effects::EffectContext context{0, buffer};

  registry.add(effectA);
  registry.add(effectB);

  // Render effect A — only fills (0,0) with red.
  registry.renderActive(context);
  TEST_ASSERT_EQUAL_UINT8(255, buffer.getPixel(0, 0).r);
  TEST_ASSERT_EQUAL_UINT8(0, buffer.getPixel(0, 0).g);

  // Switch to effect B — should trigger clear.
  TEST_ASSERT_TRUE(registry.setActiveByName("pixel-green"));
  registry.renderActive(context);

  // (1,1) should be green from effect B.
  TEST_ASSERT_EQUAL_UINT8(0, buffer.getPixel(1, 1).r);
  TEST_ASSERT_EQUAL_UINT8(255, buffer.getPixel(1, 1).g);

  // (0,0) should be black — proof that FB was cleared between effects.
  TEST_ASSERT_EQUAL_UINT8(0, buffer.getPixel(0, 0).r);
  TEST_ASSERT_EQUAL_UINT8(0, buffer.getPixel(0, 0).g);
  TEST_ASSERT_EQUAL_UINT8(0, buffer.getPixel(0, 0).b);

  // Random pixel (5,5) should also be black.
  TEST_ASSERT_EQUAL_UINT8(0, buffer.getPixel(5, 5).r);
  TEST_ASSERT_EQUAL_UINT8(0, buffer.getPixel(5, 5).g);
  TEST_ASSERT_EQUAL_UINT8(0, buffer.getPixel(5, 5).b);
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_registry_switches_active_effect_by_name);
  RUN_TEST(test_registry_rejects_unknown_effect_names);
  RUN_TEST(test_registry_clears_framebuffer_on_effect_switch);
  return UNITY_END();
}
