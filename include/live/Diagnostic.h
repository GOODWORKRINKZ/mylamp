#pragma once

#include <stdint.h>

#include <string>

namespace lamp::live {

struct Diagnostic {
  uint32_t line = 0;
  uint32_t column = 0;
  std::string message;
};

}  // namespace lamp::live