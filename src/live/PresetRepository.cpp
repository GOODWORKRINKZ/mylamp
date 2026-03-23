#include "live/PresetRepository.h"

#include "live/PresetJson.h"
#include "storage/ContentPaths.h"

namespace lamp::live {

PresetRepository::PresetRepository(storage::IFileStore& fileStore) : fileStore_(fileStore) {}

bool PresetRepository::save(const PresetModel& preset) {
  return fileStore_.writeText(storage::presetPath(preset.id), buildPresetJson(preset));
}

bool PresetRepository::load(const std::string& id, PresetModel& preset) const {
  std::string json;
  if (!fileStore_.readText(storage::presetPath(id), json)) {
    return false;
  }

  return parsePresetJson(json, preset);
}

std::vector<PresetModel> PresetRepository::list() const {
  std::vector<PresetModel> presets;
  const std::vector<std::string> paths = fileStore_.list("/presets/");
  for (const std::string& path : paths) {
    std::string json;
    if (!fileStore_.readText(path, json)) {
      continue;
    }

    PresetModel preset;
    if (parsePresetJson(json, preset)) {
      presets.push_back(preset);
    }
  }

  return presets;
}

bool PresetRepository::remove(const std::string& id) {
  return fileStore_.remove(storage::presetPath(id));
}

}  // namespace lamp::live