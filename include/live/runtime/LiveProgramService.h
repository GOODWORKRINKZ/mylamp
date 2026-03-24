#pragma once

#include <string>
#include <vector>

#include "FrameBuffer.h"
#include "live/Diagnostic.h"
#include "live/PresetModel.h"
#include "live/runtime/CompiledProgram.h"
#include "live/runtime/Compiler.h"
#include "live/runtime/Executor.h"
#include "live/runtime/RuntimeContext.h"

namespace lamp::live::runtime {

struct LiveProgramState {
  bool active = false;
  bool temporary = false;
  bool autoplayActive = false;
  std::string activePresetId;
};

class LiveProgramService {
 public:
  bool validateSource(const std::string& source, std::vector<lamp::live::Diagnostic>& diagnostics) const;
  bool runTemporary(const std::string& source, std::vector<lamp::live::Diagnostic>& diagnostics);
  bool activatePreset(const lamp::live::PresetModel& preset,
                      std::vector<lamp::live::Diagnostic>& diagnostics);
  void stop();
  void setAutoplayActive(bool active);
  LiveProgramState state() const;
  bool render(const RuntimeContext& runtimeContext, lamp::FrameBuffer& frameBuffer) const;

 private:
  bool compileSource(const std::string& source, CompiledProgram& program,
                     std::vector<lamp::live::Diagnostic>& diagnostics) const;

  Compiler compiler_;
  Executor executor_;
  CompiledProgram activeProgram_;
  LiveProgramState state_;
};

}  // namespace lamp::live::runtime