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
      "  rotate = 45\n"
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

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_parser_reads_effect_sprite_and_layers);
  RUN_TEST(test_parser_rejects_missing_effect_header_with_russian_diagnostic);
  RUN_TEST(test_parser_rejects_unknown_layer_property_with_russian_diagnostic);
  return UNITY_END();
}