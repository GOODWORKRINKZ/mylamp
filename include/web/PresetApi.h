#pragma once

#include <string>

#include "live/PresetRepository.h"
#include "live/runtime/LiveProgramService.h"

namespace lamp::web {

struct PresetApiResponse {
  int statusCode = 500;
  std::string body;
};

PresetApiResponse handleListPresetsRequest(const lamp::live::PresetRepository& repository);
PresetApiResponse handleGetPresetRequest(const lamp::live::PresetRepository& repository,
                                         const std::string& presetId);
PresetApiResponse handlePutPresetRequest(lamp::live::PresetRepository& repository,
                                         const std::string& presetId,
                                         const std::string& body);
PresetApiResponse handleDeletePresetRequest(lamp::live::PresetRepository& repository,
                                            const std::string& presetId);
PresetApiResponse handleActivatePresetRequest(
    const lamp::live::PresetRepository& repository,
    lamp::live::runtime::LiveProgramService& runtimeService, const std::string& presetId);

}  // namespace lamp::web