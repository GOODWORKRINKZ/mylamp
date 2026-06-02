#include <unity.h>

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "live/Diagnostic.h"
#include "live/dsl/Parser.h"
#include "live/runtime/Compiler.h"

namespace {

std::string readFile(const std::string& path) {
  std::ifstream file(path);
  if (!file.is_open()) return "";
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

void test_demo_effects_all_compile() {
  const char* demoFiles[] = {
    "resources/demo/nyan-cat.lux",
    "resources/demo/mario.lux",
    "resources/demo/plasma.lux",
    "resources/demo/scrolling-text.lux",
    "resources/demo/snake.lux",
    "resources/demo/fire-particles.lux",
    "resources/demo/starfield.lux",
    "resources/demo/dna.lux",
  };

  for (const char* path : demoFiles) {
    std::string source = readFile(path);
    TEST_ASSERT_FALSE_MESSAGE(source.empty(), ("Failed to read: " + std::string(path)).c_str());

    lamp::live::dsl::Program program;
    std::vector<lamp::live::Diagnostic> parseDiagnostics;
    bool parsed = lamp::live::dsl::parseProgram(source, program, parseDiagnostics);

    if (!parsed) {
      for (const auto& d : parseDiagnostics) {
        printf("  Parse error in %s line %u: %s\n", path, d.line, d.message.c_str());
      }
    }
    TEST_ASSERT_TRUE_MESSAGE(parsed, ("Parse failed for: " + std::string(path)).c_str());
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0U, static_cast<uint32_t>(parseDiagnostics.size()),
                                     ("Parse diagnostics in: " + std::string(path)).c_str());

    lamp::live::runtime::Compiler compiler;
    lamp::live::runtime::CompiledProgram compiledProgram;
    std::vector<lamp::live::Diagnostic> compileDiagnostics;
    bool compiled = compiler.compile(program, compiledProgram, compileDiagnostics);

    if (!compiled) {
      for (const auto& d : compileDiagnostics) {
        printf("  Compile error in %s line %u: %s\n", path, d.line, d.message.c_str());
      }
    }
    TEST_ASSERT_TRUE_MESSAGE(compiled, ("Compile failed for: " + std::string(path)).c_str());
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0U, static_cast<uint32_t>(compileDiagnostics.size()),
                                     ("Compile diagnostics in: " + std::string(path)).c_str());
  }
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_demo_effects_all_compile);
  return UNITY_END();
}
