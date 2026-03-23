#pragma once

#include <string>

#include "live/runtime/LiveProgramService.h"

namespace lamp::web {

struct LiveApiResponse {
  int statusCode = 500;
  std::string body;
};

LiveApiResponse handleLiveValidateRequest(lamp::live::runtime::LiveProgramService& runtimeService,
                                          const std::string& body);
LiveApiResponse handleLiveRunRequest(lamp::live::runtime::LiveProgramService& runtimeService,
                                     const std::string& body);

}  // namespace lamp::web