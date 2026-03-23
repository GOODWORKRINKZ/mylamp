#pragma once

#include <string>
#include <vector>

#include "live/Diagnostic.h"
#include "live/dsl/Ast.h"

namespace lamp::live::dsl {

bool parseProgram(const std::string& source, Program& program,
                  std::vector<lamp::live::Diagnostic>& diagnostics);

}  // namespace lamp::live::dsl