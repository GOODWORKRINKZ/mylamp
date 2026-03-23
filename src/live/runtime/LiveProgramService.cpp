#include "live/runtime/LiveProgramService.h"

#include "live/dsl/Parser.h"

namespace lamp::live::runtime {

bool LiveProgramService::runTemporary(const std::string& source,
                                      std::vector<lamp::live::Diagnostic>& diagnostics) {
  diagnostics.clear();

  CompiledProgram program;
  if (!compileSource(source, program, diagnostics)) {
    return false;
  }

  activeProgram_ = program;
  state_.active = true;
  state_.temporary = true;
  state_.autoplayActive = false;
  state_.activePresetId.clear();
  return true;
}

bool LiveProgramService::activatePreset(const lamp::live::PresetModel& preset,
                                        std::vector<lamp::live::Diagnostic>& diagnostics) {
  diagnostics.clear();

  CompiledProgram program;
  if (!compileSource(preset.source, program, diagnostics)) {
    return false;
  }

  activeProgram_ = program;
  state_.active = true;
  state_.temporary = false;
  state_.activePresetId = preset.id;
  return true;
}

void LiveProgramService::stop() {
  activeProgram_ = CompiledProgram{};
  state_ = LiveProgramState{};
}

void LiveProgramService::setAutoplayActive(bool active) {
  state_.autoplayActive = active;
}

LiveProgramState LiveProgramService::state() const {
  return state_;
}

bool LiveProgramService::render(const RuntimeContext& runtimeContext,
                                lamp::FrameBuffer& frameBuffer) const {
  if (!state_.active) {
    return false;
  }

  ExecutionContext context;
  context.timeSeconds = static_cast<float>(runtimeContext.nowMs) / 1000.0f;
  context.deltaSeconds = static_cast<float>(runtimeContext.deltaMs) / 1000.0f;
  context.temperatureC = runtimeContext.temperatureC;
  context.humidityPercent = runtimeContext.humidityPercent;
  executor_.render(activeProgram_, context, frameBuffer);
  return true;
}

bool LiveProgramService::compileSource(const std::string& source, CompiledProgram& program,
                                       std::vector<lamp::live::Diagnostic>& diagnostics) const {
  lamp::live::dsl::Program parsedProgram;
  if (!lamp::live::dsl::parseProgram(source, parsedProgram, diagnostics)) {
    return false;
  }

  return compiler_.compile(parsedProgram, program, diagnostics);
}

}  // namespace lamp::live::runtime