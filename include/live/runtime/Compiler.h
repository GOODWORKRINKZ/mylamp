#pragma once

#include <vector>

#include "live/Diagnostic.h"
#include "live/dsl/Ast.h"
#include "live/runtime/CompiledProgram.h"

namespace lamp::live::runtime {

class Compiler {
 public:
  bool compile(const dsl::Program& program, CompiledProgram& compiledProgram,
               std::vector<lamp::live::Diagnostic>& diagnostics) const;
};

}  // namespace lamp::live::runtime