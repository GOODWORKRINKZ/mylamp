#include <unity.h>

#include <string>
#include <vector>

#include "live/LiveRequestJson.h"

namespace {

void test_parse_live_validate_request_reads_source_only() {
  lamp::live::LiveRequest request;

  const bool parsed = lamp::live::parseLiveRequestJson(
      "{\"source\":\"effect warm_waves\"}", request);

  TEST_ASSERT_TRUE(parsed);
  TEST_ASSERT_EQUAL_STRING("effect warm_waves", request.source.c_str());
  TEST_ASSERT_TRUE(request.presetName.empty());
}

void test_parse_live_run_request_reads_source_and_preset_name() {
  lamp::live::LiveRequest request;

  const bool parsed = lamp::live::parseLiveRequestJson(
      "{\"source\":\"effect fire_band\",\"presetName\":\"Огонь\"}", request);

  TEST_ASSERT_TRUE(parsed);
  TEST_ASSERT_EQUAL_STRING("effect fire_band", request.source.c_str());
  TEST_ASSERT_EQUAL_STRING("Огонь", request.presetName.c_str());
}

void test_parse_live_request_rejects_missing_source() {
  lamp::live::LiveRequest request;

  TEST_ASSERT_FALSE(lamp::live::parseLiveRequestJson("{\"presetName\":\"Огонь\"}", request));
}

void test_build_diagnostic_response_contains_structured_errors() {
  std::vector<lamp::live::Diagnostic> diagnostics;
  diagnostics.push_back({4U, 9U, "Неизвестная функция hum. Возможно, ты хотел humidity()"});

  const std::string json = lamp::live::buildDiagnosticResponseJson(false, diagnostics);

  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(json.find("\"ok\":false")));
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(json.find("\"errors\":[")));
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(json.find("\"line\":4")));
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(json.find("\"column\":9")));
  TEST_ASSERT_NOT_EQUAL(
      -1,
      static_cast<int>(json.find("Неизвестная функция hum. Возможно, ты хотел humidity()")));
}

void test_build_success_diagnostic_response_uses_empty_errors_array() {
  const std::string json = lamp::live::buildDiagnosticResponseJson(true, {});

  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(json.find("\"ok\":true")));
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(json.find("\"errors\":[]")));
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_parse_live_validate_request_reads_source_only);
  RUN_TEST(test_parse_live_run_request_reads_source_and_preset_name);
  RUN_TEST(test_parse_live_request_rejects_missing_source);
  RUN_TEST(test_build_diagnostic_response_contains_structured_errors);
  RUN_TEST(test_build_success_diagnostic_response_uses_empty_errors_array);
  return UNITY_END();
}