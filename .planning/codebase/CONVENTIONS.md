# Coding Conventions

**Analysis Date:** 2026-06-02

## Naming Patterns

**Files:**
- C++ headers: `PascalCase.h` (e.g., `FrameBuffer.h`, `AppConfig.h`, `LampWebServer.h`)
- C++ sources: `PascalCase.cpp` (e.g., `FrameBuffer.cpp`, `EffectRegistry.cpp`)
- Test files: always `test_main.cpp` inside a `test_<module>/` directory
- Frontend TypeScript: `camelCase.ts` (e.g., `shellTemplate.ts`, `luxHighlight.ts`, `mockTypes.ts`)
- Frontend test files: `<name>.test.mjs` (e.g., `layout.test.mjs`, `mock-api.test.mjs`)

**Namespaces:**
- Root namespace: `lamp`
- Sub-namespaces mirror directory structure: `lamp::effects`, `lamp::live`, `lamp::live::dsl`, `lamp::live::runtime`, `lamp::network`, `lamp::storage`, `lamp::settings`, `lamp::sensors`, `lamp::time`, `lamp::update`, `lamp::web`
- Namespace closing comment includes full path: `}  // namespace lamp::effects`
- Config constants live in `lamp::config` namespace (`include/AppConfig.h`)

**Classes/Structs:**
- Classes: `PascalCase` (e.g., `FrameBuffer`, `MatrixLayout`, `EffectRegistry`, `LiveProgramService`)
- Interfaces: `I` prefix + `PascalCase` (e.g., `IEffect`, `IFileStore`, `IWiFiAdapter`, `ITimeSource`, `ISensorSource`, `IFirmwareInstaller`, `IFirmwareReleaseSource`)
- Plain data structs: `PascalCase` without prefix (e.g., `Rgb`, `EffectContext`, `SensorSample`, `PlannedNetworkState`, `BuildIdentity`, `Diagnostic`, `PresetModel`, `PlaylistModel`)
- Backend implementations: `ConcreteName + InterfaceSuffix` pattern (e.g., `ArduinoWiFiAdapter : IWiFiAdapter`, `LittleFsFileStore : IFileStore`, `PreferencesSettingsBackend : ISettingsBackend`)

**Functions/Methods:**
- C++: `camelCase` (e.g., `setActiveByName`, `toLinearIndex`, `isReady`, `mapPanelPixel`, `planStartup`)
- C++ free functions: `camelCase` (e.g., `parseProgram`, `loadSettingsIfReady`, `saveSettingsIfReady`, `parseLiveRequestJson`, `buildDiagnosticResponseJson`)
- TypeScript: `camelCase` (e.g., `renderShellMarkup`, `getEditorValue`, `buildPresetId`, `readScenarioFromUrl`)

**Variables:**
- C++ member variables: `snake_case_` with trailing underscore (e.g., `layout_`, `pixels_`, `active_`, `fileStore_`, `server_`, `snapshot_`)
- C++ local variables: `snake_case` or `camelCase`
- C++ constants: `k` prefix + PascalCase for global constexpr (e.g., `kPixelCount`, `kPanelWidth`, `kInvalidIndex`, `kLedDataPin`); UPPER_CASE for macro defines (e.g., `APP_VERSION`, `APP_CHANNEL`)
- TypeScript: `camelCase` for variables, `PascalCase` for types/interfaces

**Types/Interfaces (TypeScript):**
- Types and interfaces: `PascalCase` (e.g., `ScenarioId`, `StatusPayload`, `LiveDiagnostic`, `ShellTemplateOptions`, `StarterSnippet`)
- Exported types are in `src/dev/mockTypes.ts`

**Enum Values:**
- C++ enum classes: `k` prefix + PascalCase (e.g., `NetworkMode::kAccessPoint`, `FirmwareUpdateState::kIdle`, `FirmwareUpdateState::kInstalling`)

## Code Style

**Formatting:**
- No formatter configuration detected (no `.clang-format`, `.prettierrc`, `.eslintrc`, `biome.json`)
- Manual formatting with consistent indentation (2 spaces in C++, 2 spaces in TypeScript)
- Opening brace on same line for namespaces, classes, functions
- Consistent blank line between methods in class definitions

**Linting:**
- No linter configuration detected for C++ or TypeScript
- TypeScript `strict: true` enabled in `frontend/tsconfig.json`
- TypeScript `isolatedModules: true` enabled

**Header Guards:**
- All headers use `#pragma once` (no `#ifndef` guards)

## Import Organization

**C++ Includes:**
1. Standard library headers (e.g., `<stdint.h>`, `<string>`, `<vector>`)
2. Project headers from `include/` (e.g., `"FrameBuffer.h"`, `"AppConfig.h"`)
3. Subdirectory includes grouped (e.g., `"effects/EffectRegistry.h"`, `"live/runtime/Executor.h"`)
4. Blank line between groups
- No relative `../` includes — all paths relative to `include/` root

**TypeScript Imports:**
1. CSS styles: `import "./styles/app.css"`
2. Local modules with `./` prefix
3. Type-only imports use `import type` syntax (e.g., `import type { LiveDiagnosticResponse } from "./dev/mockTypes"`)
- Path aliases: none configured

**Frontend Build:**
- Vite resolves `src/` as the TypeScript root
- Built output goes to `resources/dist/` for embedding in firmware

## Error Handling

**C++ Patterns:**
- Functions return `bool` for success/failure (e.g., `setActiveByName`, `save`, `load`, `tokenize`, `compile`, `parseProgram`)
- Error details accumulated in `std::vector<Diagnostic>` output parameter (DSL parsing/compilation)
- Error messages in Russian for end-user facing diagnostics (DSL live coding)
- Error details accumulated in `std::string& error` output parameter (firmware update install)
- Null checks on pointer parameters before use (e.g., `if (effectName == nullptr)` in `EffectRegistry::setActiveByName`)
- `isReady()` guard pattern: storage and service methods check readiness before I/O
- Silent returns (no logging) on invalid input (e.g., `FrameBuffer::setPixel` returns if index invalid)
- No exceptions used — return codes and output parameters throughout

**TypeScript Patterns:**
- Guards at entry points (e.g., `if (!app) throw new Error("App root not found")`)
- `try/catch` with `process.exitCode = 1` in test files
- API calls use `fetch` with response status checks
- User-facing error messages in Russian

## Logging

**Framework:** `Serial` (Arduino `Serial.print`/`Serial.println`)
- No structured logging framework
- Debug output via `Serial.println` in `main.cpp`

**Patterns:**
- Log on startup: version, board, hardware type
- Log on state transitions (WiFi connect/disconnect, effect changes)
- Log heartbeat and FPS at intervals
- No logging from library/service code — only from `main.cpp` orchestration layer

## Comments

**When to Comment:**
- DSL grammar hints in snippet definition files
- Section headers in `main.cpp` (orchestration setup)
- Minimal comments in library code — code should be self-documenting
- Russian language comments in frontend UI components

**JSDoc/TSDoc:**
- Occasional JSDoc for exported functions in TypeScript (e.g., `luxHighlight.ts`: `/** Lux language syntax highlighter... */`)

## Function Design

**Size:** Functions tend to be small and focused (5-30 lines typical)
- Single responsibility: parsers parse, compilers compile, executors execute
- Rendering effects are separated from effect registration

**Parameters:**
- Output parameters passed by reference (non-const) as last arguments (e.g., `compile(program, compiledProgram, diagnostics)`)
- Input structs passed by const reference (e.g., `const CompiledProgram&`, `const ExecutionContext&`)
- Dependencies injected via constructor references (e.g., `IFileStore&`, `IFirmwareReleaseSource&`)

**Return Values:**
- `bool` for operations that can fail
- `void` for operations that can't fail (e.g., `clear()`, `fill()`, `render()`)
- `const char*` for string identity (effect names)
- Objects returned by value for simple structs (e.g., `SensorSample read()`, `PlannedNetworkState plan()`)

## Module Design

**Exports:**
- One class/struct per header file (e.g., `FrameBuffer.h` contains only `FrameBuffer`)
- Exception: Config constants grouped in `AppConfig.h`
- Interface + implementation separated: `.h` in `include/`, `.cpp` in `src/`
- Free functions in headers where they serve as module API (e.g., `parseProgram`, `loadSettingsIfReady`)
- TypeScript: one concern per file in `src/`

**Barrel Files:** Not used

## Module Organization

**C++ Module Layers:**
```
include/effects/   → Effect contracts (IEffect.h) and registry
include/live/      → Live coding DSL, presets, playlists
include/live/dsl/  → DSL parser/lexer AST definitions
include/live/runtime/ → Compiled program execution
include/network/   → WiFi abstraction and planning
include/sensors/   → Sensor reading interface
include/settings/  → Settings persistence and access
include/storage/   → File system abstraction
include/time/      → NTP time sync abstraction
include/update/    → Firmware OTA update pipeline
include/web/       → HTTP server and JSON API handlers
```

**Frontend Module Layers:**
```
src/main.ts        → Application entry point, UI orchestration
src/ui/            → HTML template rendering (shellTemplate.ts)
src/editor/        → Code editor features (help, highlighting, snippets)
src/dev/           → Mock backend scenarios for local development
src/styles/        → CSS stylesheet
```

**Dependency direction:** Frontend has no framework. TypeScript modules import from each other directly. C++ modules use dependency injection via interfaces — concrete implementations (Arduino-specific) are wired in `main.cpp`.

---

*Convention analysis: 2026-06-02*
