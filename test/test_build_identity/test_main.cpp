#include <unity.h>

namespace {

void test_hardware_type_macro_is_defined_for_current_target() {
#ifdef APP_HARDWARE_TYPE
  TEST_ASSERT_EQUAL_STRING("c3-cylinder32x16", APP_HARDWARE_TYPE);
#else
  TEST_FAIL_MESSAGE("APP_HARDWARE_TYPE is not defined");
#endif
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_hardware_type_macro_is_defined_for_current_target);
  return UNITY_END();
}