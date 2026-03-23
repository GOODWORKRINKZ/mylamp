#pragma once

#include <string>
#include <vector>

#include "live/Diagnostic.h"

namespace lamp::live {

struct LiveRequest {
  std::string source;
  std::string presetName;
};

bool parseLiveRequestJson(const std::string& json, LiveRequest& request);
std::string buildDiagnosticResponseJson(bool ok, const std::vector<Diagnostic>& diagnostics);

}  // namespace lamp::live