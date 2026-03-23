#include "web/LiveApi.h"

#include "live/LiveRequestJson.h"

namespace lamp::web {

namespace {

lamp::live::Diagnostic makeDiagnostic(uint32_t line, uint32_t column, const char* message) {
  lamp::live::Diagnostic diagnostic;
  diagnostic.line = line;
  diagnostic.column = column;
  diagnostic.message = message;
  return diagnostic;
}

LiveApiResponse makeDiagnosticResponse(int statusCode,
                                       const std::vector<lamp::live::Diagnostic>& diagnostics) {
  LiveApiResponse response;
  response.statusCode = statusCode;
  response.body = lamp::live::buildDiagnosticResponseJson(statusCode == 200, diagnostics);
  return response;
}

LiveApiResponse makeInvalidJsonResponse() {
  std::vector<lamp::live::Diagnostic> diagnostics;
  diagnostics.push_back(makeDiagnostic(0U, 0U, "Некорректный JSON запроса"));
  return makeDiagnosticResponse(400, diagnostics);
}

}  // namespace

LiveApiResponse handleLiveValidateRequest(lamp::live::runtime::LiveProgramService& runtimeService,
                                          const std::string& body) {
  lamp::live::LiveRequest request;
  if (!lamp::live::parseLiveRequestJson(body, request)) {
    return makeInvalidJsonResponse();
  }

  std::vector<lamp::live::Diagnostic> diagnostics;
  if (!runtimeService.validateSource(request.source, diagnostics)) {
    return makeDiagnosticResponse(400, diagnostics);
  }

  return makeDiagnosticResponse(200, diagnostics);
}

LiveApiResponse handleLiveRunRequest(lamp::live::runtime::LiveProgramService& runtimeService,
                                     const std::string& body) {
  lamp::live::LiveRequest request;
  if (!lamp::live::parseLiveRequestJson(body, request)) {
    return makeInvalidJsonResponse();
  }

  std::vector<lamp::live::Diagnostic> diagnostics;
  if (!runtimeService.runTemporary(request.source, diagnostics)) {
    return makeDiagnosticResponse(400, diagnostics);
  }

  return makeDiagnosticResponse(200, diagnostics);
}

}  // namespace lamp::web