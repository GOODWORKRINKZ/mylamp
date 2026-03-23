#include "live/runtime/PlaylistScheduler.h"

namespace lamp::live::runtime {

namespace {

lamp::live::Diagnostic makeDiagnostic(const std::string& message) {
  lamp::live::Diagnostic diagnostic;
  diagnostic.line = 0;
  diagnostic.column = 0;
  diagnostic.message = message;
  return diagnostic;
}

}  // namespace

bool PlaylistScheduler::start(const lamp::live::PlaylistModel& playlist,
                              const lamp::live::PresetRepository& presetRepository,
                              LiveProgramService& runtimeService,
                              std::vector<lamp::live::Diagnostic>& diagnostics) {
  diagnostics.clear();
  activePlaylist_ = playlist;
  state_ = PlaylistSchedulerState{};
  elapsedMs_ = 0;

  const size_t firstEntry = findNextEnabledEntry(0U);
  if (firstEntry >= activePlaylist_.entries.size()) {
    diagnostics.push_back(makeDiagnostic("В плейлисте нет активных preset entries"));
    return false;
  }

  if (!activateEntry(firstEntry, presetRepository, runtimeService, diagnostics)) {
    return false;
  }

  state_.active = true;
  state_.autoplayActive = true;
  state_.activePlaylistId = activePlaylist_.id;
  return true;
}

bool PlaylistScheduler::advance(uint32_t deltaMs, const lamp::live::PresetRepository& presetRepository,
                                LiveProgramService& runtimeService,
                                std::vector<lamp::live::Diagnostic>& diagnostics) {
  if (!state_.active || activePlaylist_.entries.empty()) {
    return false;
  }

  elapsedMs_ += deltaMs;
  const uint32_t currentDurationMs =
      activePlaylist_.entries[state_.activeEntryIndex].durationSec * 1000U;
  if (currentDurationMs == 0U || elapsedMs_ < currentDurationMs) {
    return true;
  }

  elapsedMs_ = 0U;
  size_t nextIndex = findNextEnabledEntry(static_cast<size_t>(state_.activeEntryIndex) + 1U);
  if (nextIndex >= activePlaylist_.entries.size()) {
    if (!activePlaylist_.repeat) {
      runtimeService.setAutoplayActive(false);
      stop();
      return true;
    }
    nextIndex = findNextEnabledEntry(0U);
  }

  return activateEntry(nextIndex, presetRepository, runtimeService, diagnostics);
}

void PlaylistScheduler::stop() {
  activePlaylist_ = lamp::live::PlaylistModel{};
  state_ = PlaylistSchedulerState{};
  elapsedMs_ = 0U;
}

void PlaylistScheduler::syncWithRuntime(const LiveProgramService& runtimeService) {
  if (state_.active && !runtimeService.state().autoplayActive) {
    stop();
  }
}

PlaylistSchedulerState PlaylistScheduler::state() const {
  return state_;
}

bool PlaylistScheduler::activateEntry(size_t entryIndex, const lamp::live::PresetRepository& presetRepository,
                                      LiveProgramService& runtimeService,
                                      std::vector<lamp::live::Diagnostic>& diagnostics) {
  if (entryIndex >= activePlaylist_.entries.size()) {
    return false;
  }

  lamp::live::PresetModel preset;
  if (!presetRepository.load(activePlaylist_.entries[entryIndex].presetId, preset)) {
    diagnostics.push_back(makeDiagnostic("Не найден preset для playlist entry"));
    return false;
  }
  if (!runtimeService.activatePreset(preset, diagnostics)) {
    return false;
  }

  runtimeService.setAutoplayActive(true);
  state_.active = true;
  state_.autoplayActive = true;
  state_.activePlaylistId = activePlaylist_.id;
  state_.activeEntryIndex = static_cast<uint16_t>(entryIndex);
  return true;
}

size_t PlaylistScheduler::findNextEnabledEntry(size_t startIndex) const {
  for (size_t index = startIndex; index < activePlaylist_.entries.size(); ++index) {
    if (activePlaylist_.entries[index].enabled) {
      return index;
    }
  }
  return activePlaylist_.entries.size();
}

}  // namespace lamp::live::runtime