#include <unity.h>

#include <string>
#include <vector>

#include "live/dsl/Parser.h"

namespace {

void test_parser_reads_effect_sprite_and_layers() {
  const std::string source =
      "effect \"flying_shapes\"\n"
      "\n"
      "sprite heart {\n"
      "  bitmap \"\"\"\n"
      "  ..##..\n"
      "  .####.\n"
      "  ######\n"
      "  \"\"\"\n"
      "}\n"
      "\n"
      "layer heart1 {\n"
      "  use heart\n"
      "  color rgb(255, 40, 80)\n"
      "  x = 4 + t * 2\n"
      "  y = 8 + sin(t * 1.5) * 3\n"
      "  scale = 1.0 + sin(t * 3.0) * 0.25\n"
      "  rotation = t * 0.5\n"
      "  blend = add\n"
      "  visible = 1\n"
      "}\n";

  lamp::live::dsl::Program program;
  std::vector<lamp::live::Diagnostic> diagnostics;

  const bool parsed = lamp::live::dsl::parseProgram(source, program, diagnostics);

  TEST_ASSERT_TRUE(parsed);
  TEST_ASSERT_EQUAL_UINT32(0U, static_cast<uint32_t>(diagnostics.size()));
  TEST_ASSERT_EQUAL_STRING("flying_shapes", program.effectName.c_str());
  TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(program.sprites.size()));
  TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(program.layers.size()));
  TEST_ASSERT_EQUAL_STRING("heart", program.sprites[0].name.c_str());
  TEST_ASSERT_EQUAL_STRING("..##..\n.####.\n######", program.sprites[0].bitmap.c_str());
  TEST_ASSERT_EQUAL_STRING("heart1", program.layers[0].name.c_str());
  TEST_ASSERT_EQUAL_STRING("heart", program.layers[0].spriteName.c_str());
  TEST_ASSERT_EQUAL_STRING("rgb(255, 40, 80)", program.layers[0].colorExpression.c_str());
  TEST_ASSERT_EQUAL_STRING("4 + t * 2", program.layers[0].xExpression.c_str());
  TEST_ASSERT_EQUAL_STRING("8 + sin(t * 1.5) * 3", program.layers[0].yExpression.c_str());
  TEST_ASSERT_EQUAL_STRING("1.0 + sin(t * 3.0) * 0.25",
                           program.layers[0].scaleExpression.c_str());
  TEST_ASSERT_EQUAL_STRING("t * 0.5", program.layers[0].rotationExpression.c_str());
  TEST_ASSERT_EQUAL_STRING("add", program.layers[0].blendMode.c_str());
  TEST_ASSERT_EQUAL_STRING("1", program.layers[0].visibleExpression.c_str());
}

void test_parser_rejects_missing_effect_header_with_russian_diagnostic() {
  const std::string source =
      "sprite heart {\n"
      "  bitmap \"\"\"\n"
      "  ##\n"
      "  \"\"\"\n"
      "}\n";

  lamp::live::dsl::Program program;
  std::vector<lamp::live::Diagnostic> diagnostics;

  const bool parsed = lamp::live::dsl::parseProgram(source, program, diagnostics);

  TEST_ASSERT_FALSE(parsed);
  TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(diagnostics.size()));
  TEST_ASSERT_NOT_EQUAL(-1,
                        static_cast<int>(diagnostics[0].message.find("Ожидалось объявление effect")));
}

void test_parser_rejects_unknown_layer_property_with_russian_diagnostic() {
  const std::string source =
      "effect \"broken\"\n"
      "layer heart1 {\n"
      "  use heart\n"
      "  opacity = 0.5\n"
      "}\n";

  lamp::live::dsl::Program program;
  std::vector<lamp::live::Diagnostic> diagnostics;

  const bool parsed = lamp::live::dsl::parseProgram(source, program, diagnostics);

  TEST_ASSERT_FALSE(parsed);
  TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(diagnostics.size()));
  TEST_ASSERT_NOT_EQUAL(
      -1,
      static_cast<int>(diagnostics[0].message.find("Свойство слоя не поддерживается в v1")));
}

void test_parser_reads_text_declaration() {
  const std::string source =
      "effect \"text_demo\"\n"
      "\n"
      "text greeting \"HELLO\"\n"
      "\n"
      "layer label {\n"
      "  use greeting\n"
      "  color rgb(255, 255, 255)\n"
      "  x = 0\n"
      "  y = 0\n"
      "  scale = 1\n"
      "  visible = 1\n"
      "}\n";

  lamp::live::dsl::Program program;
  std::vector<lamp::live::Diagnostic> diagnostics;

  const bool parsed = lamp::live::dsl::parseProgram(source, program, diagnostics);

  TEST_ASSERT_TRUE(parsed);
  TEST_ASSERT_EQUAL_UINT32(0U, static_cast<uint32_t>(diagnostics.size()));
  TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(program.texts.size()));
  TEST_ASSERT_EQUAL_STRING("greeting", program.texts[0].name.c_str());
  TEST_ASSERT_EQUAL_STRING("HELLO", program.texts[0].content.c_str());
  TEST_ASSERT_EQUAL_UINT32(0U, static_cast<uint32_t>(program.sprites.size()));
  TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(program.layers.size()));
  TEST_ASSERT_EQUAL_STRING("greeting", program.layers[0].spriteName.c_str());
}

void test_parser_parses_sprite_with_multiple_frames() {
  const std::string source =
      "effect \"anim\"\n"
      "sprite cat {\n"
      "  frame walk1 {\n"
      "    bitmap \"\"\"\n"
      "    ##\n"
      "    \"\"\"\n"
      "  }\n"
      "  frame walk2 {\n"
      "    bitmap \"\"\"\n"
      "    ..\n"
      "    \"\"\"\n"
      "  }\n"
      "}\n"
      "layer cat1 {\n"
      "  use cat\n"
      "  color rgb(255, 255, 255)\n"
      "  x = 0\n"
      "  y = 0\n"
      "  frame = (t * 4) % 2\n"
      "  visible = 1\n"
      "}\n";

  lamp::live::dsl::Program program;
  std::vector<lamp::live::Diagnostic> diagnostics;

  const bool parsed = lamp::live::dsl::parseProgram(source, program, diagnostics);

  TEST_ASSERT_TRUE(parsed);
  TEST_ASSERT_EQUAL_UINT32(0U, static_cast<uint32_t>(diagnostics.size()));
  TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(program.sprites.size()));
  TEST_ASSERT_EQUAL_STRING("cat", program.sprites[0].name.c_str());
  TEST_ASSERT_EQUAL_UINT32(2U, static_cast<uint32_t>(program.sprites[0].frames.size()));
  TEST_ASSERT_EQUAL_STRING("walk1", program.sprites[0].frames[0].name.c_str());
  TEST_ASSERT_EQUAL_STRING("##", program.sprites[0].frames[0].bitmap.c_str());
  TEST_ASSERT_EQUAL_STRING("walk2", program.sprites[0].frames[1].name.c_str());
  TEST_ASSERT_EQUAL_STRING("..", program.sprites[0].frames[1].bitmap.c_str());
  TEST_ASSERT_EQUAL_STRING("(t * 4) % 2", program.layers[0].frameExpression.c_str());
}

void test_parser_parses_for_loop_with_layers() {
  const std::string source =
      "effect \"looped\"\n"
      "sprite dot {\n"
      "  bitmap \"\"\"\n"
      "  #\n"
      "  \"\"\"\n"
      "}\n"
      "for i = 0; i < 3; i = i + 1 {\n"
      "  layer s {\n"
      "    use dot\n"
      "    color rgb(255, 255, 255)\n"
      "    x = i * 5\n"
      "    y = 0\n"
      "    visible = 1\n"
      "  }\n"
      "}\n";

  lamp::live::dsl::Program program;
  std::vector<lamp::live::Diagnostic> diagnostics;

  const bool parsed = lamp::live::dsl::parseProgram(source, program, diagnostics);

  TEST_ASSERT_TRUE(parsed);
  TEST_ASSERT_EQUAL_UINT32(0U, static_cast<uint32_t>(diagnostics.size()));
  TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(program.forLoops.size()));
  TEST_ASSERT_EQUAL_STRING("i", program.forLoops[0].loopVariable.c_str());
  TEST_ASSERT_EQUAL_STRING("0", program.forLoops[0].startExpression.c_str());
  TEST_ASSERT_EQUAL_STRING("3", program.forLoops[0].endExpression.c_str());
  TEST_ASSERT_EQUAL_STRING("1", program.forLoops[0].stepExpression.c_str());
  TEST_ASSERT_EQUAL_STRING("<", program.forLoops[0].comparisonOperator.c_str());
  TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(program.forLoops[0].body.size()));
  TEST_ASSERT_EQUAL_STRING("s", program.forLoops[0].body[0].name.c_str());
  TEST_ASSERT_EQUAL_STRING("dot", program.forLoops[0].body[0].spriteName.c_str());
  TEST_ASSERT_EQUAL_STRING("i * 5", program.forLoops[0].body[0].xExpression.c_str());
}

void test_parser_parses_layer_with_frame_expression() {
  const std::string source =
      "effect \"framed\"\n"
      "sprite icon {\n"
      "  frame a {\n"
      "    bitmap \"\"\"\n"
      "    #\n"
      "    \"\"\"\n"
      "  }\n"
      "  frame b {\n"
      "    bitmap \"\"\"\n"
      "    .\n"
      "    \"\"\"\n"
      "  }\n"
      "}\n"
      "layer icon1 {\n"
      "  use icon\n"
      "  color rgb(255, 255, 255)\n"
      "  x = 0\n"
      "  y = 0\n"
      "  frame = (t * 3) % 2\n"
      "  visible = 1\n"
      "}\n";

  lamp::live::dsl::Program program;
  std::vector<lamp::live::Diagnostic> diagnostics;

  const bool parsed = lamp::live::dsl::parseProgram(source, program, diagnostics);

  TEST_ASSERT_TRUE(parsed);
  TEST_ASSERT_EQUAL_UINT32(0U, static_cast<uint32_t>(diagnostics.size()));
  TEST_ASSERT_EQUAL_UINT32(2U, static_cast<uint32_t>(program.sprites[0].frames.size()));
  TEST_ASSERT_EQUAL_STRING("(t * 3) % 2", program.layers[0].frameExpression.c_str());
}

void test_parser_rejects_for_loop_with_reserved_variable() {
  const std::string source =
      "effect \"bad\"\n"
      "sprite dot {\n"
      "  bitmap \"\"\"\n"
      "  #\n"
      "  \"\"\"\n"
      "}\n"
      "for t = 0; t < 3; t = t + 1 {\n"
      "  layer s {\n"
      "    use dot\n"
      "    color rgb(255, 255, 255)\n"
      "    x = 0\n"
      "    y = 0\n"
      "    visible = 1\n"
      "  }\n"
      "}\n";

  lamp::live::dsl::Program program;
  std::vector<lamp::live::Diagnostic> diagnostics;

  const bool parsed = lamp::live::dsl::parseProgram(source, program, diagnostics);

  // Should fail because 't' is a reserved word
  TEST_ASSERT_FALSE(parsed);
  TEST_ASSERT_NOT_EQUAL(
      -1,
      static_cast<int>(diagnostics[0].message.find("не может совпадать со встроенным")));
}

void test_parser_rejects_for_loop_with_non_integer_bounds() {
  const std::string source =
      "effect \"bad2\"\n"
      "sprite dot {\n"
      "  bitmap \"\"\"\n"
      "  #\n"
      "  \"\"\"\n"
      "}\n"
      "for i = 0; i < t; i = i + 1 {\n"
      "  layer s {\n"
      "    use dot\n"
      "    color rgb(255, 255, 255)\n"
      "    x = 0\n"
      "    y = 0\n"
      "    visible = 1\n"
      "  }\n"
      "}\n";

  lamp::live::dsl::Program program;
  std::vector<lamp::live::Diagnostic> diagnostics;

  const bool parsed = lamp::live::dsl::parseProgram(source, program, diagnostics);

  // Should fail because 't' is not an integer constant
  TEST_ASSERT_FALSE(parsed);
  TEST_ASSERT_NOT_EQUAL(
      -1,
      static_cast<int>(diagnostics[0].message.find("должны быть целыми числами")));
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_parser_reads_effect_sprite_and_layers);
  RUN_TEST(test_parser_rejects_missing_effect_header_with_russian_diagnostic);
  RUN_TEST(test_parser_rejects_unknown_layer_property_with_russian_diagnostic);
  RUN_TEST(test_parser_reads_text_declaration);
  RUN_TEST(test_parser_parses_sprite_with_multiple_frames);
  RUN_TEST(test_parser_parses_for_loop_with_layers);
  RUN_TEST(test_parser_parses_layer_with_frame_expression);
  RUN_TEST(test_parser_rejects_for_loop_with_reserved_variable);
  RUN_TEST(test_parser_rejects_for_loop_with_non_integer_bounds);
  return UNITY_END();
}