#pragma once

#include <string>

#include "live/PresetModel.h"

namespace lamp::live {

std::string buildPresetJson(const PresetModel& preset);
bool parsePresetJson(const std::string& json, PresetModel& preset);

}  // namespace lamp::live