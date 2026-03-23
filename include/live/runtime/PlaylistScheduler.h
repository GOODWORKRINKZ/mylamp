#pragma once

#include <stdint.h>

#include <string>
#include <vector>

#include "live/Diagnostic.h"
#include "live/PlaylistModel.h"
#include "live/PresetRepository.h"
#include "live/runtime/LiveProgramService.h"

namespace lamp::live::runtime {

struct PlaylistSchedulerState {
  bool active = false;
  bool autoplayActive = false;
  std::string activePlaylistId;
  uint16_t activeEntryIndex = 0;
};

class PlaylistScheduler {
 public:
  bool start(const lamp::live::PlaylistModel& playlist, const lamp::live::PresetRepository& presetRepository,
             LiveProgramService& runtimeService, std::vector<lamp::live::Diagnostic>& diagnostics);
  bool advance(uint32_t deltaMs, const lamp::live::PresetRepository& presetRepository,
               LiveProgramService& runtimeService, std::vector<lamp::live::Diagnostic>& diagnostics);
  void stop();
  void syncWithRuntime(const LiveProgramService& runtimeService);
  PlaylistSchedulerState state() const;

 private:
  bool activateEntry(size_t entryIndex, const lamp::live::PresetRepository& presetRepository,
                     LiveProgramService& runtimeService,
                     std::vector<lamp::live::Diagnostic>& diagnostics);
  size_t findNextEnabledEntry(size_t startIndex) const;

  lamp::live::PlaylistModel activePlaylist_;
  PlaylistSchedulerState state_;
  uint32_t elapsedMs_ = 0;
};

}  // namespace lamp::live::runtime