#pragma once

#include <string>
#include <vector>

#include "live/Diagnostic.h"
#include "live/dsl/Token.h"

namespace lamp::live::dsl {

class Lexer {
 public:
  bool tokenize(const std::string& source, std::vector<Token>& tokens,
                std::vector<lamp::live::Diagnostic>& diagnostics) const;
};

}  // namespace lamp::live::dsl