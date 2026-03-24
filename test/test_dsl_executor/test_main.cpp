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
  return UNITY_END();
}