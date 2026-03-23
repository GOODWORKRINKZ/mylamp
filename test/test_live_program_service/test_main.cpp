#include <unity.h>

#include <string>
#include <vector>

#include "FrameBuffer.h"
#include "MatrixLayout.h"
#include "live/Diagnostic.h"
#include "live/PresetModel.h"
#include "live/runtime/LiveProgramService.h"
#include "live/runtime/RuntimeContext.h"

namespace {

const char* kSimpleSource =
    "effect \"dot\"\n"
    "sprite dot {\n"
    "  bitmap \"\"\"\n"
    "  #\n"
    "  \"\"\"\n"
    "}\n"
    "layer dot1 {\n"
    "  use dot\n"
    "  color rgb(200, 40, 10)\n"
    "  x = 1\n"
    "  y = 2\n"
    "  scale = 1\n"
    "  visible = 1\n"
    "}\n";

lamp::Rgb renderPixel(lamp::live::runtime::LiveProgramService& service, uint16_t x, uint16_t y) {
  lamp::MatrixLayout layout;
  lamp::FrameBuffer frameBuffer(layout);
  lamp::live::runtime::RuntimeContext runtimeContext;
  runtimeContext.nowMs = 1000U;
  runtimeContext.deltaMs = 16U;
  runtimeContext.temperatureC = 22.0f;
  runtimeContext.humidityPercent = 50.0f;
  TEST_ASSERT_TRUE(service.render(runtimeContext, frameBuffer));
  return frameBuffer.getPixel(static_cast<int16_t>(x), static_cast<int16_t>(y));
}

void test_temporary_run_activates_program_and_renders() {
  lamp::live::runtime::LiveProgramService service;
  std::vector<lamp::live::Diagnostic> diagnostics;

  TEST_ASSERT_TRUE(service.runTemporary(kSimpleSource, diagnostics));
  TEST_ASSERT_EQUAL_UINT32(0U, static_cast<uint32_t>(diagnostics.size()));

  const lamp::live::runtime::LiveProgramState state = service.state();
  TEST_ASSERT_TRUE(state.active);
  TEST_ASSERT_TRUE(state.temporary);
  TEST_ASSERT_FALSE(state.autoplayActive);
  TEST_ASSERT_EQUAL_STRING("", state.activePresetId.c_str());

  const lamp::Rgb pixel = renderPixel(service, 1U, 2U);
  TEST_ASSERT_EQUAL_UINT8(200U, pixel.r);
  TEST_ASSERT_EQUAL_UINT8(40U, pixel.g);
  TEST_ASSERT_EQUAL_UINT8(10U, pixel.b);
}

void test_activate_preset_sets_active_preset_state() {
  lamp::live::runtime::LiveProgramService service;
  lamp::live::PresetModel preset;
  preset.id = "warm-dot";
  preset.name = "Warm Dot";
  preset.source = kSimpleSource;

  std::vector<lamp::live::Diagnostic> diagnostics;
  TEST_ASSERT_TRUE(service.activatePreset(preset, diagnostics));
  TEST_ASSERT_EQUAL_UINT32(0U, static_cast<uint32_t>(diagnostics.size()));

  const lamp::live::runtime::LiveProgramState state = service.state();
  TEST_ASSERT_TRUE(state.active);
  TEST_ASSERT_FALSE(state.temporary);
  TEST_ASSERT_EQUAL_STRING("warm-dot", state.activePresetId.c_str());
}

void test_stop_program_clears_active_state_and_rendering() {
  lamp::live::runtime::LiveProgramService service;
  std::vector<lamp::live::Diagnostic> diagnostics;
  TEST_ASSERT_TRUE(service.runTemporary(kSimpleSource, diagnostics));

  service.stop();

  const lamp::live::runtime::LiveProgramState state = service.state();
  TEST_ASSERT_FALSE(state.active);
  TEST_ASSERT_FALSE(state.temporary);
  TEST_ASSERT_FALSE(state.autoplayActive);
  TEST_ASSERT_EQUAL_STRING("", state.activePresetId.c_str());

  lamp::MatrixLayout layout;
  lamp::FrameBuffer frameBuffer(layout);
  lamp::live::runtime::RuntimeContext runtimeContext;
  TEST_ASSERT_FALSE(service.render(runtimeContext, frameBuffer));
}

void test_manual_run_stops_autoplay_immediately() {
  lamp::live::runtime::LiveProgramService service;
  service.setAutoplayActive(true);

  std::vector<lamp::live::Diagnostic> diagnostics;
  TEST_ASSERT_TRUE(service.runTemporary(kSimpleSource, diagnostics));

  const lamp::live::runtime::LiveProgramState state = service.state();
  TEST_ASSERT_TRUE(state.active);
  TEST_ASSERT_FALSE(state.autoplayActive);
  TEST_ASSERT_TRUE(state.temporary);
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_temporary_run_activates_program_and_renders);
  RUN_TEST(test_activate_preset_sets_active_preset_state);
  RUN_TEST(test_stop_program_clears_active_state_and_rendering);
  RUN_TEST(test_manual_run_stops_autoplay_immediately);
  return UNITY_END();
}