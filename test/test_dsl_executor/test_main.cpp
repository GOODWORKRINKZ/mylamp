#include <unity.h>

#include <string>
#include <vector>

#include "FrameBuffer.h"
#include "MatrixLayout.h"
#include "live/Diagnostic.h"
#include "live/dsl/Parser.h"
#include "live/runtime/Compiler.h"
#include "live/runtime/Executor.h"

namespace {

bool compileSource(const std::string& source, lamp::live::runtime::CompiledProgram& compiledProgram,
                   std::vector<lamp::live::Diagnostic>& diagnostics) {
  lamp::live::dsl::Program program;
  if (!lamp::live::dsl::parseProgram(source, program, diagnostics)) {
    return false;
  }

  lamp::live::runtime::Compiler compiler;
  return compiler.compile(program, compiledProgram, diagnostics);
}

void test_executor_projects_sprite_bitmap_to_framebuffer() {
  const std::string source =
      "effect \"dot\"\n"
      "sprite dot {\n"
      "  bitmap \"\"\"\n"
      "  #\n"
      "  \"\"\"\n"
      "}\n"
      "layer dot1 {\n"
      "  use dot\n"
      "  color rgb(255, 10, 20)\n"
      "  x = 2\n"
      "  y = 3\n"
      "  scale = 1\n"
      "  rotation = 0\n"
      "  visible = 1\n"
      "}\n";

  lamp::live::runtime::CompiledProgram compiledProgram;
  std::vector<lamp::live::Diagnostic> diagnostics;
  TEST_ASSERT_TRUE(compileSource(source, compiledProgram, diagnostics));

  lamp::MatrixLayout layout;
  lamp::FrameBuffer frameBuffer(layout);
  lamp::live::runtime::Executor executor;
  lamp::live::runtime::ExecutionContext context;
  executor.render(compiledProgram, context, frameBuffer);

  const lamp::Rgb pixel = frameBuffer.getPixel(2, 3);
  TEST_ASSERT_EQUAL_UINT8(255U, pixel.r);
  TEST_ASSERT_EQUAL_UINT8(10U, pixel.g);
  TEST_ASSERT_EQUAL_UINT8(20U, pixel.b);
}

void test_executor_applies_layer_order_last_layer_wins() {
  const std::string source =
      "effect \"stack\"\n"
      "sprite dot {\n"
      "  bitmap \"\"\"\n"
      "  #\n"
      "  \"\"\"\n"
      "}\n"
      "layer bottom {\n"
      "  use dot\n"
      "  color rgb(255, 0, 0)\n"
      "  x = 5\n"
      "  y = 4\n"
      "  scale = 1\n"
      "  rotation = 0\n"
      "  visible = 1\n"
      "}\n"
      "layer top {\n"
      "  use dot\n"
      "  color rgb(0, 0, 255)\n"
      "  x = 5\n"
      "  y = 4\n"
      "  scale = 1\n"
      "  rotation = 0\n"
      "  visible = 1\n"
      "}\n";

  lamp::live::runtime::CompiledProgram compiledProgram;
  std::vector<lamp::live::Diagnostic> diagnostics;
  TEST_ASSERT_TRUE(compileSource(source, compiledProgram, diagnostics));

  lamp::MatrixLayout layout;
  lamp::FrameBuffer frameBuffer(layout);
  lamp::live::runtime::Executor executor;
  lamp::live::runtime::ExecutionContext context;
  executor.render(compiledProgram, context, frameBuffer);

  const lamp::Rgb pixel = frameBuffer.getPixel(5, 4);
  TEST_ASSERT_EQUAL_UINT8(0U, pixel.r);
  TEST_ASSERT_EQUAL_UINT8(0U, pixel.g);
  TEST_ASSERT_EQUAL_UINT8(255U, pixel.b);
}

void test_executor_applies_animated_x_y_and_scale() {
  const std::string source =
      "effect \"moving\"\n"
      "sprite dot {\n"
      "  bitmap \"\"\"\n"
      "  #\n"
      "  \"\"\"\n"
      "}\n"
      "layer mover {\n"
      "  use dot\n"
      "  color rgb(20, 220, 40)\n"
      "  x = 1 + t * 2\n"
      "  y = 2 + t\n"
      "  scale = 1 + t\n"
      "  rotation = 0\n"
      "  visible = 1\n"
      "}\n";

  lamp::live::runtime::CompiledProgram compiledProgram;
  std::vector<lamp::live::Diagnostic> diagnostics;
  TEST_ASSERT_TRUE(compileSource(source, compiledProgram, diagnostics));

  lamp::MatrixLayout layout;
  lamp::FrameBuffer frameBuffer(layout);
  lamp::live::runtime::Executor executor;
  lamp::live::runtime::ExecutionContext context;
  context.timeSeconds = 1.0f;
  executor.render(compiledProgram, context, frameBuffer);

  const lamp::Rgb topLeft = frameBuffer.getPixel(3, 3);
  const lamp::Rgb bottomRight = frameBuffer.getPixel(4, 4);
  TEST_ASSERT_EQUAL_UINT8(20U, topLeft.r);
  TEST_ASSERT_EQUAL_UINT8(220U, topLeft.g);
  TEST_ASSERT_EQUAL_UINT8(40U, topLeft.b);
  TEST_ASSERT_EQUAL_UINT8(20U, bottomRight.r);
  TEST_ASSERT_EQUAL_UINT8(220U, bottomRight.g);
  TEST_ASSERT_EQUAL_UINT8(40U, bottomRight.b);
}

void test_executor_uses_sensor_function_stubs() {
  const std::string source =
      "effect \"sensor\"\n"
      "sprite dot {\n"
      "  bitmap \"\"\"\n"
      "  #\n"
      "  \"\"\"\n"
      "}\n"
      "layer sensorDot {\n"
      "  use dot\n"
      "  color rgb(0, 120, 255)\n"
      "  x = temp() / 10\n"
      "  y = humidity() / 20\n"
      "  scale = 1\n"
      "  rotation = 0\n"
      "  visible = 1\n"
      "}\n";

  lamp::live::runtime::CompiledProgram compiledProgram;
  std::vector<lamp::live::Diagnostic> diagnostics;
  TEST_ASSERT_TRUE(compileSource(source, compiledProgram, diagnostics));

  lamp::MatrixLayout layout;
  lamp::FrameBuffer frameBuffer(layout);
  lamp::live::runtime::Executor executor;
  lamp::live::runtime::ExecutionContext context;
  context.temperatureC = 30.0f;
  context.humidityPercent = 40.0f;
  executor.render(compiledProgram, context, frameBuffer);

  const lamp::Rgb pixel = frameBuffer.getPixel(3, 2);
  TEST_ASSERT_EQUAL_UINT8(0U, pixel.r);
  TEST_ASSERT_EQUAL_UINT8(120U, pixel.g);
  TEST_ASSERT_EQUAL_UINT8(255U, pixel.b);
}

void test_executor_evaluates_modulo_operator() {
  const std::string source =
      "effect \"mod\"\n"
      "sprite dot {\n"
      "  bitmap \"\"\"\n"
      "  #\n"
      "  \"\"\"\n"
      "}\n"
      "layer mod1 {\n"
      "  use dot\n"
      "  color rgb(100, 50, 25)\n"
      "  x = 7 % 3\n"
      "  y = 10 % 4\n"
      "  scale = 1\n"
      "  rotation = 0\n"
      "  visible = 1\n"
      "}\n";

  lamp::live::runtime::CompiledProgram compiledProgram;
  std::vector<lamp::live::Diagnostic> diagnostics;
  TEST_ASSERT_TRUE(compileSource(source, compiledProgram, diagnostics));
  TEST_ASSERT_EQUAL_UINT32(0U, static_cast<uint32_t>(diagnostics.size()));

  lamp::MatrixLayout layout;
  lamp::FrameBuffer frameBuffer(layout);
  lamp::live::runtime::Executor executor;
  lamp::live::runtime::ExecutionContext context;
  executor.render(compiledProgram, context, frameBuffer);

  // 7 % 3 = 1, 10 % 4 = 2
  const lamp::Rgb pixel = frameBuffer.getPixel(1, 2);
  TEST_ASSERT_EQUAL_UINT8(100U, pixel.r);
  TEST_ASSERT_EQUAL_UINT8(50U, pixel.g);
  TEST_ASSERT_EQUAL_UINT8(25U, pixel.b);
}

void test_compiler_reports_line_number_in_expression_error() {
  const std::string source =
      "effect \"bad\"\n"
      "sprite dot {\n"
      "  bitmap \"\"\"\n"
      "  #\n"
      "  \"\"\"\n"
      "}\n"
      "layer broken {\n"
      "  use dot\n"
      "  color rgb(255, 0, 0)\n"
      "  x = 1 + bogus_var\n"
      "  y = 0\n"
      "  scale = 1\n"
      "  rotation = 0\n"
      "  visible = 1\n"
      "}\n";

  lamp::live::runtime::CompiledProgram compiledProgram;
  std::vector<lamp::live::Diagnostic> diagnostics;
  TEST_ASSERT_FALSE(compileSource(source, compiledProgram, diagnostics));
  TEST_ASSERT_TRUE(diagnostics.size() >= 1U);
  // "x = 1 + bogus_var" is on line 10
  TEST_ASSERT_EQUAL_UINT32(10U, diagnostics[0].line);
}

void test_executor_rotates_sprite_around_its_center() {
  const std::string source =
      "effect \"rotate\"\n"
      "sprite bar {\n"
      "  bitmap \"\"\"\n"
      "  ##\n"
      "  \"\"\"\n"
      "}\n"
      "layer bar1 {\n"
      "  use bar\n"
      "  color rgb(70, 140, 210)\n"
      "  x = 10\n"
      "  y = 5\n"
      "  scale = 1\n"
      "  rotation = 1.5707963\n"
      "  visible = 1\n"
      "}\n";

  lamp::live::runtime::CompiledProgram compiledProgram;
  std::vector<lamp::live::Diagnostic> diagnostics;
  TEST_ASSERT_TRUE(compileSource(source, compiledProgram, diagnostics));
  TEST_ASSERT_EQUAL_UINT32(0U, static_cast<uint32_t>(diagnostics.size()));

  lamp::MatrixLayout layout;
  lamp::FrameBuffer frameBuffer(layout);
  lamp::live::runtime::Executor executor;
  lamp::live::runtime::ExecutionContext context;
  executor.render(compiledProgram, context, frameBuffer);

  const lamp::Rgb upperPixel = frameBuffer.getPixel(11, 5);
  const lamp::Rgb lowerPixel = frameBuffer.getPixel(11, 6);
  TEST_ASSERT_EQUAL_UINT8(70U, upperPixel.r);
  TEST_ASSERT_EQUAL_UINT8(140U, upperPixel.g);
  TEST_ASSERT_EQUAL_UINT8(210U, upperPixel.b);
  TEST_ASSERT_EQUAL_UINT8(70U, lowerPixel.r);
  TEST_ASSERT_EQUAL_UINT8(140U, lowerPixel.g);
  TEST_ASSERT_EQUAL_UINT8(210U, lowerPixel.b);

  const lamp::Rgb originalLeft = frameBuffer.getPixel(10, 5);
  const lamp::Rgb originalRight = frameBuffer.getPixel(11, 5);
  TEST_ASSERT_EQUAL_UINT8(0U, originalLeft.r);
  TEST_ASSERT_EQUAL_UINT8(0U, originalLeft.g);
  TEST_ASSERT_EQUAL_UINT8(0U, originalLeft.b);
  TEST_ASSERT_EQUAL_UINT8(70U, originalRight.r);
}

void test_executor_add_blend_accumulates_color() {
  const std::string source =
      "effect \"additive\"\n"
      "sprite dot {\n"
      "  bitmap \"\"\"\n"
      "  #\n"
      "  \"\"\"\n"
      "}\n"
      "layer base {\n"
      "  use dot\n"
      "  color rgb(120, 40, 10)\n"
      "  x = 7\n"
      "  y = 6\n"
      "  scale = 1\n"
      "  rotation = 0\n"
      "  visible = 1\n"
      "}\n"
      "layer glow {\n"
      "  use dot\n"
      "  color rgb(150, 80, 250)\n"
      "  x = 7\n"
      "  y = 6\n"
      "  scale = 1\n"
      "  rotation = 0\n"
      "  blend = add\n"
      "  visible = 1\n"
      "}\n";

  lamp::live::runtime::CompiledProgram compiledProgram;
  std::vector<lamp::live::Diagnostic> diagnostics;
  TEST_ASSERT_TRUE(compileSource(source, compiledProgram, diagnostics));

  lamp::MatrixLayout layout;
  lamp::FrameBuffer frameBuffer(layout);
  lamp::live::runtime::Executor executor;
  lamp::live::runtime::ExecutionContext context;
  executor.render(compiledProgram, context, frameBuffer);

  const lamp::Rgb pixel = frameBuffer.getPixel(7, 6);
  TEST_ASSERT_EQUAL_UINT8(255U, pixel.r);
  TEST_ASSERT_EQUAL_UINT8(120U, pixel.g);
  TEST_ASSERT_EQUAL_UINT8(255U, pixel.b);
}

void test_executor_multiply_blend_modulates_existing_color() {
  const std::string source =
      "effect \"multiply\"\n"
      "sprite dot {\n"
      "  bitmap \"\"\"\n"
      "  #\n"
      "  \"\"\"\n"
      "}\n"
      "layer base {\n"
      "  use dot\n"
      "  color rgb(200, 100, 50)\n"
      "  x = 4\n"
      "  y = 8\n"
      "  scale = 1\n"
      "  rotation = 0\n"
      "  visible = 1\n"
      "}\n"
      "layer mask {\n"
      "  use dot\n"
      "  color rgb(128, 255, 64)\n"
      "  x = 4\n"
      "  y = 8\n"
      "  scale = 1\n"
      "  rotation = 0\n"
      "  blend = multiply\n"
      "  visible = 1\n"
      "}\n";

  lamp::live::runtime::CompiledProgram compiledProgram;
  std::vector<lamp::live::Diagnostic> diagnostics;
  TEST_ASSERT_TRUE(compileSource(source, compiledProgram, diagnostics));

  lamp::MatrixLayout layout;
  lamp::FrameBuffer frameBuffer(layout);
  lamp::live::runtime::Executor executor;
  lamp::live::runtime::ExecutionContext context;
  executor.render(compiledProgram, context, frameBuffer);

  const lamp::Rgb pixel = frameBuffer.getPixel(4, 8);
  TEST_ASSERT_EQUAL_UINT8(100U, pixel.r);
  TEST_ASSERT_EQUAL_UINT8(100U, pixel.g);
  TEST_ASSERT_EQUAL_UINT8(12U, pixel.b);
}

void test_compiler_rejects_unknown_blend_mode_with_line_number() {
  const std::string source =
      "effect \"bad_blend\"\n"
      "sprite dot {\n"
      "  bitmap \"\"\"\n"
      "  #\n"
      "  \"\"\"\n"
      "}\n"
      "layer broken {\n"
      "  use dot\n"
      "  color rgb(255, 0, 0)\n"
      "  x = 0\n"
      "  y = 0\n"
      "  scale = 1\n"
      "  rotation = 0\n"
      "  blend = screen\n"
      "  visible = 1\n"
      "}\n";

  lamp::live::runtime::CompiledProgram compiledProgram;
  std::vector<lamp::live::Diagnostic> diagnostics;
  TEST_ASSERT_FALSE(compileSource(source, compiledProgram, diagnostics));
  TEST_ASSERT_TRUE(diagnostics.size() >= 1U);
  TEST_ASSERT_EQUAL_UINT32(14U, diagnostics[0].line);
}

void test_executor_renders_text_as_sprite() {
  const std::string source =
      "effect \"text_test\"\n"
      "text msg \"AB\"\n"
      "layer label {\n"
      "  use msg\n"
      "  color rgb(200, 100, 50)\n"
      "  x = 0\n"
      "  y = 0\n"
      "  scale = 1\n"
      "  visible = 1\n"
      "}\n";

  lamp::live::runtime::CompiledProgram compiledProgram;
  std::vector<lamp::live::Diagnostic> diagnostics;
  TEST_ASSERT_TRUE(compileSource(source, compiledProgram, diagnostics));
  TEST_ASSERT_EQUAL_UINT32(0U, static_cast<uint32_t>(diagnostics.size()));

  lamp::MatrixLayout layout;
  lamp::FrameBuffer frameBuffer(layout);
  lamp::live::runtime::Executor executor;
  lamp::live::runtime::ExecutionContext context;
  executor.render(compiledProgram, context, frameBuffer);

  // 'A' glyph: .#. / #.# / ### / #.# / #.#
  // Top-center pixel of 'A' at (1, 0) should be lit
  const lamp::Rgb topA = frameBuffer.getPixel(1, 0);
  TEST_ASSERT_EQUAL_UINT8(200U, topA.r);
  TEST_ASSERT_EQUAL_UINT8(100U, topA.g);
  TEST_ASSERT_EQUAL_UINT8(50U, topA.b);

  // Top-left pixel of 'A' at (0, 0) should be empty
  const lamp::Rgb emptyA = frameBuffer.getPixel(0, 0);
  TEST_ASSERT_EQUAL_UINT8(0U, emptyA.r);

  // Gap between 'A' and 'B' at column 3 should be empty
  const lamp::Rgb gap = frameBuffer.getPixel(3, 0);
  TEST_ASSERT_EQUAL_UINT8(0U, gap.r);

  // 'B' starts at column 4. Glyph: ##. / #.# / ##. / #.# / ##.
  // Top-left pixel of 'B' at (4, 0) should be lit
  const lamp::Rgb topB = frameBuffer.getPixel(4, 0);
  TEST_ASSERT_EQUAL_UINT8(200U, topB.r);
  TEST_ASSERT_EQUAL_UINT8(100U, topB.g);
  TEST_ASSERT_EQUAL_UINT8(50U, topB.b);
}

void test_expression_at_depth_limit_succeeds() {
  // Generate sin(sin(...sin(t)...)) nested exactly 63 deep (under the 64 limit).
  constexpr int kDepth = 63;
  std::string expr = "t";
  for (int i = 0; i < kDepth; ++i) {
    expr = "sin(" + expr + ")";
  }

  const std::string source =
      "effect \"deep_ok\"\n"
      "sprite dot {\n"
      "  bitmap \"\"\"\n"
      "  #\n"
      "  \"\"\"\n"
      "}\n"
      "layer d1 {\n"
      "  use dot\n"
      "  color rgb(100, 0, 0)\n"
      "  x = " + expr + "\n"
      "  y = 0\n"
      "  scale = 1\n"
      "  rotation = 0\n"
      "  visible = 1\n"
      "}\n";

  lamp::live::runtime::CompiledProgram compiledProgram;
  std::vector<lamp::live::Diagnostic> diagnostics;
  TEST_ASSERT_TRUE(compileSource(source, compiledProgram, diagnostics));

  lamp::MatrixLayout layout;
  lamp::FrameBuffer frameBuffer(layout);
  lamp::live::runtime::Executor executor;
  lamp::live::runtime::ExecutionContext context;
  context.timeSeconds = 0.5f;
  executor.render(compiledProgram, context, frameBuffer);

  // sin(0.5) ≈ 0.479... after 63 more sin() calls converges near 0.
  // The sprite pixel should be rendered somewhere; the key is that render didn't crash.
  // We just verify at least one pixel was lit (not all black due to crash/clamp).
  bool anyLit = false;
  for (uint16_t i = 0; i < frameBuffer.size(); ++i) {
    const lamp::Rgb p = frameBuffer.pixelAtIndex(i);
    if (p.r > 0 || p.g > 0 || p.b > 0) {
      anyLit = true;
      break;
    }
  }
  TEST_ASSERT_TRUE(anyLit);
}

void test_deeply_nested_expression_does_not_overflow() {
  // Generate sin(sin(...sin(t)...)) nested 70 deep (exceeds the 64 limit).
  constexpr int kDepth = 70;
  std::string expr = "t";
  for (int i = 0; i < kDepth; ++i) {
    expr = "sin(" + expr + ")";
  }

  const std::string source =
      "effect \"deep_bad\"\n"
      "sprite dot {\n"
      "  bitmap \"\"\"\n"
      "  #\n"
      "  \"\"\"\n"
      "}\n"
      "layer d1 {\n"
      "  use dot\n"
      "  color rgb(0, 100, 0)\n"
      "  x = " + expr + "\n"
      "  y = 0\n"
      "  scale = 1\n"
      "  rotation = 0\n"
      "  visible = 1\n"
      "}\n";

  lamp::live::runtime::CompiledProgram compiledProgram;
  std::vector<lamp::live::Diagnostic> diagnostics;
  TEST_ASSERT_TRUE(compileSource(source, compiledProgram, diagnostics));

  lamp::MatrixLayout layout;
  lamp::FrameBuffer frameBuffer(layout);
  lamp::live::runtime::Executor executor;
  lamp::live::runtime::ExecutionContext context;
  context.timeSeconds = 0.5f;
  // This must NOT crash (stack overflow → SIGSEGV).
  executor.render(compiledProgram, context, frameBuffer);
  // If we reach here, the depth guard worked.
}

void test_executor_renders_correct_sprite_frame() {
  const std::string source =
      "effect \"frames\"\n"
      "sprite cat {\n"
      "  frame walk1 {\n"
      "    bitmap \"\"\"\n"
      "    #.\n"
      "    \"\"\"\n"
      "  }\n"
      "  frame walk2 {\n"
      "    bitmap \"\"\"\n"
      "    .#\n"
      "    \"\"\"\n"
      "  }\n"
      "}\n"
      "layer cat1 {\n"
      "  use cat\n"
      "  color rgb(255, 255, 255)\n"
      "  x = 0\n"
      "  y = 0\n"
      "  scale = 1\n"
      "  frame = 0\n"
      "  visible = 1\n"
      "}\n";

  lamp::live::runtime::CompiledProgram compiledProgram;
  std::vector<lamp::live::Diagnostic> diagnostics;
  TEST_ASSERT_TRUE(compileSource(source, compiledProgram, diagnostics));

  lamp::MatrixLayout layout;
  lamp::FrameBuffer frameBuffer(layout);
  lamp::live::runtime::Executor executor;
  lamp::live::runtime::ExecutionContext context;
  executor.render(compiledProgram, context, frameBuffer);

  // Frame 0: pixel at (0,0) should be lit, (1,0) empty
  const lamp::Rgb px0 = frameBuffer.getPixel(0, 0);
  TEST_ASSERT_EQUAL_UINT8(255U, px0.r);
  const lamp::Rgb px1 = frameBuffer.getPixel(1, 0);
  TEST_ASSERT_EQUAL_UINT8(0U, px1.r);
}

void test_executor_frame_expression_modulo() {
  // 3-frame sprite, frame = (t * 4) % 3 — verify cycling
  const std::string source =
      "effect \"cycle\"\n"
      "sprite tri {\n"
      "  frame a {\n"
      "    bitmap \"\"\"\n"
      "    #..\n"
      "    \"\"\"\n"
      "  }\n"
      "  frame b {\n"
      "    bitmap \"\"\"\n"
      "    .#.\n"
      "    \"\"\"\n"
      "  }\n"
      "  frame c {\n"
      "    bitmap \"\"\"\n"
      "    ..#\n"
      "    \"\"\"\n"
      "  }\n"
      "}\n"
      "layer t1 {\n"
      "  use tri\n"
      "  color rgb(255, 255, 255)\n"
      "  x = 0\n"
      "  y = 0\n"
      "  scale = 1\n"
      "  frame = (t * 4) % 3\n"
      "  visible = 1\n"
      "}\n";

  lamp::live::runtime::CompiledProgram compiledProgram;
  std::vector<lamp::live::Diagnostic> diagnostics;
  TEST_ASSERT_TRUE(compileSource(source, compiledProgram, diagnostics));

  lamp::MatrixLayout layout;
  lamp::FrameBuffer frameBuffer(layout);
  lamp::live::runtime::Executor executor;

  // t=0: frame = 0 → pixel at (0,0)
  {
    lamp::live::runtime::ExecutionContext context;
    context.timeSeconds = 0.0f;
    executor.render(compiledProgram, context, frameBuffer);
    TEST_ASSERT_EQUAL_UINT8(255U, frameBuffer.getPixel(0, 0).r);
    TEST_ASSERT_EQUAL_UINT8(0U, frameBuffer.getPixel(1, 0).r);
  }

  // t=0.5: frame = 2 % 3 = 2 → pixel at (2,0)
  {
    lamp::live::runtime::ExecutionContext context;
    context.timeSeconds = 0.5f;
    frameBuffer.clear();
    executor.render(compiledProgram, context, frameBuffer);
    TEST_ASSERT_EQUAL_UINT8(255U, frameBuffer.getPixel(2, 0).r);
    TEST_ASSERT_EQUAL_UINT8(0U, frameBuffer.getPixel(0, 0).r);
  }
}

void test_executor_for_loop_unrolls_layers() {
  const std::string source =
      "effect \"unrolled\"\n"
      "sprite dot {\n"
      "  bitmap \"\"\"\n"
      "  #\n"
      "  \"\"\"\n"
      "}\n"
      "for i = 0; i < 3; i = i + 1 {\n"
      "  layer s {\n"
      "    use dot\n"
      "    color rgb(100, 0, 0)\n"
      "    x = i * 5\n"
      "    y = 0\n"
      "    scale = 1\n"
      "    visible = 1\n"
      "  }\n"
      "}\n";

  lamp::live::runtime::CompiledProgram compiledProgram;
  std::vector<lamp::live::Diagnostic> diagnostics;
  TEST_ASSERT_TRUE(compileSource(source, compiledProgram, diagnostics));
  TEST_ASSERT_EQUAL_UINT32(0U, static_cast<uint32_t>(diagnostics.size()));

  // 3 unrolled layers
  TEST_ASSERT_EQUAL_UINT32(3U, static_cast<uint32_t>(compiledProgram.layers.size()));
  TEST_ASSERT_EQUAL_STRING("s_0", compiledProgram.layers[0].name.c_str());
  TEST_ASSERT_EQUAL_STRING("s_1", compiledProgram.layers[1].name.c_str());
  TEST_ASSERT_EQUAL_STRING("s_2", compiledProgram.layers[2].name.c_str());

  lamp::MatrixLayout layout;
  lamp::FrameBuffer frameBuffer(layout);
  lamp::live::runtime::Executor executor;
  lamp::live::runtime::ExecutionContext context;
  executor.render(compiledProgram, context, frameBuffer);

  // Layer s_0 at x=0, s_1 at x=5, s_2 at x=10
  TEST_ASSERT_EQUAL_UINT8(100U, frameBuffer.getPixel(0, 0).r);
  TEST_ASSERT_EQUAL_UINT8(100U, frameBuffer.getPixel(5, 0).r);
  TEST_ASSERT_EQUAL_UINT8(100U, frameBuffer.getPixel(10, 0).r);
  // Position at x=3 should be empty
  TEST_ASSERT_EQUAL_UINT8(0U, frameBuffer.getPixel(3, 0).r);
}

void test_executor_single_frame_sprite_still_works() {
  // Old-style single-bitmap sprite — backward compat (D-04)
  const std::string source =
      "effect \"oldschool\"\n"
      "sprite dot {\n"
      "  bitmap \"\"\"\n"
      "  #\n"
      "  \"\"\"\n"
      "}\n"
      "layer dot1 {\n"
      "  use dot\n"
      "  color rgb(50, 200, 100)\n"
      "  x = 5\n"
      "  y = 7\n"
      "  scale = 1\n"
      "  rotation = 0\n"
      "  visible = 1\n"
      "}\n";

  lamp::live::runtime::CompiledProgram compiledProgram;
  std::vector<lamp::live::Diagnostic> diagnostics;
  TEST_ASSERT_TRUE(compileSource(source, compiledProgram, diagnostics));
  TEST_ASSERT_EQUAL_UINT32(0U, static_cast<uint32_t>(diagnostics.size()));

  lamp::MatrixLayout layout;
  lamp::FrameBuffer frameBuffer(layout);
  lamp::live::runtime::Executor executor;
  lamp::live::runtime::ExecutionContext context;
  executor.render(compiledProgram, context, frameBuffer);

  const lamp::Rgb pixel = frameBuffer.getPixel(5, 7);
  TEST_ASSERT_EQUAL_UINT8(50U, pixel.r);
  TEST_ASSERT_EQUAL_UINT8(200U, pixel.g);
  TEST_ASSERT_EQUAL_UINT8(100U, pixel.b);
}

void test_executor_frame_index_bounded() {
  // 2-frame sprite, frame=99 → renders frame 1 (99 % 2 = 1)
  const std::string source =
      "effect \"bounded\"\n"
      "sprite two {\n"
      "  frame a {\n"
      "    bitmap \"\"\"\n"
      "    #.\n"
      "    \"\"\"\n"
      "  }\n"
      "  frame b {\n"
      "    bitmap \"\"\"\n"
      "    .#\n"
      "    \"\"\"\n"
      "  }\n"
      "}\n"
      "layer t1 {\n"
      "  use two\n"
      "  color rgb(255, 255, 255)\n"
      "  x = 0\n"
      "  y = 0\n"
      "  scale = 1\n"
      "  frame = 99\n"
      "  visible = 1\n"
      "}\n";

  lamp::live::runtime::CompiledProgram compiledProgram;
  std::vector<lamp::live::Diagnostic> diagnostics;
  TEST_ASSERT_TRUE(compileSource(source, compiledProgram, diagnostics));

  lamp::MatrixLayout layout;
  lamp::FrameBuffer frameBuffer(layout);
  lamp::live::runtime::Executor executor;
  lamp::live::runtime::ExecutionContext context;
  executor.render(compiledProgram, context, frameBuffer);

  // Frame 1: pixel at (1,0) should be lit (99 % 2 = 1)
  TEST_ASSERT_EQUAL_UINT8(0U, frameBuffer.getPixel(0, 0).r);
  TEST_ASSERT_EQUAL_UINT8(255U, frameBuffer.getPixel(1, 0).r);
}

void test_executor_max_unrolled_layers_enforced() {
  // Create a for-loop that exceeds kMaxUnrolledLayers (64)
  std::string source =
      "effect \"overflow\"\n"
      "sprite dot {\n"
      "  bitmap \"\"\"\n"
      "  #\n"
      "  \"\"\"\n"
      "}\n"
      "for i = 0; i < 70; i = i + 1 {\n"
      "  layer s {\n"
      "    use dot\n"
      "    color rgb(255, 0, 0)\n"
      "    x = 0\n"
      "    y = 0\n"
      "    scale = 1\n"
      "    visible = 1\n"
      "  }\n"
      "}\n";

  lamp::live::runtime::CompiledProgram compiledProgram;
  std::vector<lamp::live::Diagnostic> diagnostics;
  TEST_ASSERT_FALSE(compileSource(source, compiledProgram, diagnostics));
  TEST_ASSERT_TRUE(diagnostics.size() >= 1U);
  // Should contain "Слишком много слоёв" diagnostic
  TEST_ASSERT_NOT_EQUAL(
      -1,
      static_cast<int>(diagnostics[0].message.find("Слишком много слоёв после развёртки for")));
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_executor_projects_sprite_bitmap_to_framebuffer);
  RUN_TEST(test_executor_applies_layer_order_last_layer_wins);
  RUN_TEST(test_executor_applies_animated_x_y_and_scale);
  RUN_TEST(test_executor_uses_sensor_function_stubs);
  RUN_TEST(test_executor_evaluates_modulo_operator);
  RUN_TEST(test_compiler_reports_line_number_in_expression_error);
  RUN_TEST(test_executor_rotates_sprite_around_its_center);
  RUN_TEST(test_executor_add_blend_accumulates_color);
  RUN_TEST(test_executor_multiply_blend_modulates_existing_color);
  RUN_TEST(test_compiler_rejects_unknown_blend_mode_with_line_number);
  RUN_TEST(test_executor_renders_text_as_sprite);
  RUN_TEST(test_expression_at_depth_limit_succeeds);
  RUN_TEST(test_deeply_nested_expression_does_not_overflow);
  RUN_TEST(test_executor_renders_correct_sprite_frame);
  RUN_TEST(test_executor_frame_expression_modulo);
  RUN_TEST(test_executor_for_loop_unrolls_layers);
  RUN_TEST(test_executor_single_frame_sprite_still_works);
  RUN_TEST(test_executor_frame_index_bounded);
  RUN_TEST(test_executor_max_unrolled_layers_enforced);
  return UNITY_END();
}