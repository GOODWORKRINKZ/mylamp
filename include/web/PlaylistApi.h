#pragma once

#include <string>
#include <vector>

#include "live/Diagnostic.h"
#include "live/PlaylistRepository.h"
#include "live/PresetRepository.h"
#include "live/runtime/LiveProgramService.h"
#include "live/runtime/PlaylistScheduler.h"

namespace lamp::web {

struct PlaylistApiResponse {
  int statusCode = 500;
  std::string body;
};

PlaylistApiResponse handleListPlaylistsRequest(const lamp::live::PlaylistRepository& repository);
PlaylistApiResponse handlePutPlaylistRequest(lamp::live::PlaylistRepository& repository,
                                             const std::string& playlistId,
                                             const std::string& body);
PlaylistApiResponse handleDeletePlaylistRequest(lamp::live::PlaylistRepository& repository,
                                                const std::string& playlistId);
PlaylistApiResponse handleStartPlaylistRequest(
    const lamp::live::PlaylistRepository& playlistRepository,
    const lamp::live::PresetRepository& presetRepository,
    lamp::live::runtime::PlaylistScheduler& scheduler,
    lamp::live::runtime::LiveProgramService& runtimeService, const std::string& playlistId,
    std::vector<lamp::live::Diagnostic>& diagnostics);
PlaylistApiResponse handleStopPlaylistRequest(lamp::live::runtime::PlaylistScheduler& scheduler,
                                              lamp::live::runtime::LiveProgramService& runtimeService);

}  // namespace lamp::web