#include <unity.h>

#include <string>
#include <vector>

#include "live/runtime/LiveProgramService.h"
#include "web/LiveApi.h"

namespace {

const char* kValidSource =
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

void test_live_validate_returns_ok_for_valid_source() {
  lamp::live::runtime::LiveProgramService service;

  const lamp::web::LiveApiResponse response =
      lamp::web::handleLiveValidateRequest(service, std::string("{\"source\":\"") +
                                                        "effect \\\"dot\\\"\\n"
                                                        "sprite dot {\\n"
                                                        "  bitmap \\\"\\\"\\\"\\n"
                                                        "  #\\n"
                                                        "  \\\"\\\"\\\"\\n"
                                                        "}\\n"
                                                        "layer dot1 {\\n"
                                                        "  use dot\\n"
                                                        "  color rgb(200, 40, 10)\\n"
                                                        "  x = 1\\n"
                                                        "  y = 2\\n"
                                                        "  scale = 1\\n"
                                                        "  visible = 1\\n"
                                                        "}\\n\"}");

  TEST_ASSERT_EQUAL_INT(200, response.statusCode);
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(response.body.find("\"ok\":true")));
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(response.body.find("\"errors\":[]")));
}

void test_live_validate_returns_diagnostics_for_invalid_source() {
  lamp::live::runtime::LiveProgramService service;

  const lamp::web::LiveApiResponse response =
      lamp::web::handleLiveValidateRequest(service,
                                           "{\"source\":\"effect \\\"broken\\\"\\nlayer x {\\n  rotate = 45\\n}\\n\"}");

  TEST_ASSERT_EQUAL_INT(400, response.statusCode);
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(response.body.find("\"ok\":false")));
  TEST_ASSERT_NOT_EQUAL(-1,
                        static_cast<int>(response.body.find("Свойство слоя не поддерживается в v1")));
}

void test_live_run_activates_program_for_valid_source() {
  lamp::live::runtime::LiveProgramService service;

  const lamp::web::LiveApiResponse response =
      lamp::web::handleLiveRunRequest(service, std::string("{\"source\":\"") +
                                                   "effect \\\"dot\\\"\\n"
                                                   "sprite dot {\\n"
                                                   "  bitmap \\\"\\\"\\\"\\n"
                                                   "  #\\n"
                                                   "  \\\"\\\"\\\"\\n"
                                                   "}\\n"
                                                   "layer dot1 {\\n"
                                                   "  use dot\\n"
                                                   "  color rgb(200, 40, 10)\\n"
                                                   "  x = 1\\n"
                                                   "  y = 2\\n"
                                                   "  scale = 1\\n"
                                                   "  visible = 1\\n"
                                                   "}\\n\"}");

  TEST_ASSERT_EQUAL_INT(200, response.statusCode);
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(response.body.find("\"ok\":true")));
  TEST_ASSERT_TRUE(service.state().active);
  TEST_ASSERT_TRUE(service.state().temporary);
}

void test_live_run_rejects_invalid_json() {
  lamp::live::runtime::LiveProgramService service;

  const lamp::web::LiveApiResponse response = lamp::web::handleLiveRunRequest(service, "{bad json}");

  TEST_ASSERT_EQUAL_INT(400, response.statusCode);
  TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(response.body.find("Некорректный JSON запроса")));
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_live_validate_returns_ok_for_valid_source);
  RUN_TEST(test_live_validate_returns_diagnostics_for_invalid_source);
  RUN_TEST(test_live_run_activates_program_for_valid_source);
  RUN_TEST(test_live_run_rejects_invalid_json);
  return UNITY_END();
}