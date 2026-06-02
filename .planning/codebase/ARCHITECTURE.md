<!-- refreshed: 2026-06-02 -->
# Architecture

**Analysis Date:** 2026-06-02

## System Overview

```text
┌──────────────────────────────────────────────────────────────────────┐
│                        Web Control Surface                           │
│   `frontend/` (Vite/TS)  ──HTTP──▶  `src/web/` (ESP32 WebServer)    │
├──────────────────┬──────────────────┬───────────────────────────────┤
│   Live Coding    │   Settings UI    │     OTA / Status              │
│   `LiveApi`      │   `Network*,     │     `FirmwareUpdateService`   │
│                  │    Time*`        │     `StatusJsonBuilder`       │
└────────┬─────────┴────────┬─────────┴──────────┬────────────────────┘
         │                  │                     │
         ▼                  ▼                     ▼
┌──────────────────────────────────────────────────────────────────────┐
│                    Runtime Services Layer                            │
│   `src/live/runtime/`  `src/network/`  `src/time/`  `src/sensors/`  │
│   `src/settings/`      `src/update/`   `src/storage/`               │
├──────────────────────────────────────────────────────────────────────┤
│                    Rendering Pipeline                                │
│   `LiveProgramService` ──▶ `FrameBuffer` ──▶ `MatrixLayout`         │
│   `EffectRegistry`     ──▶ `FrameBuffer` ──▶ `MatrixLayout`         │
│   `ClockOverlay`       ──▶ `FrameBuffer` (overlay pass)             │
└──────────────────────────────────────────────────────────────────────┘
         │
         ▼
┌──────────────────────────────────────────────────────────────────────┐
│  Hardware Abstraction                                                │
│  `MatrixLayout` (logical→physical mapping)  `FastLED` (CRGB output)  │
│  `IWiFiAdapter` `ISensorSource` `ITimeSource` `IFileStore`           │
└──────────────────────────────────────────────────────────────────────┘
```

## Component Responsibilities

| Component | Responsibility | File |
|-----------|----------------|------|
| `LiveProgramService` | Compile, validate, and execute DSL programs; manage preset activation | `include/live/runtime/LiveProgramService.h` |
| `PlaylistScheduler` | Sequence preset playback with timing per entry | `include/live/runtime/PlaylistScheduler.h` |
| `EffectRegistry` | Register compile-time effects, manage active effect | `include/effects/EffectRegistry.h` |
| `FrameBuffer` | Logical pixel buffer (32×16 RGBA), rendering target | `include/FrameBuffer.h` |
| `MatrixLayout` | Logical coordinate → physical LED index mapping, panel serpentine | `include/MatrixLayout.h` |
| `WiFiManager` | AP/client mode startup, IP acquisition | `include/network/WiFiManager.h` |
| `NetworkPlanner` | Derive operational policy (liveCodingAllowed, otaAllowed, etc.) from network state | `include/network/NetworkPlanner.h` |
| `TimePlanner` | Derive clock policy (NTP sync, overlay visibility) from settings & network state | `include/time/TimePlanner.h` |
| `TimeRuntimeService` | Periodically sync NTP, produce formatted time string | `include/time/TimeRuntimeService.h` |
| `SensorRuntimeService` | Poll temperature/humidity sensor, track stale reads | `include/sensors/SensorRuntimeService.h` |
| `FirmwareUpdateService` | Check GitHub Releases for OTA updates, trigger install | `include/update/FirmwareUpdateService.h` |
| `LampWebServer` | HTTP server: serve SPA, REST API endpoints for all subsystems | `include/web/LampWebServer.h` |
| `PresetRepository` | CRUD for presets on LittleFS (JSON files) | `include/live/PresetRepository.h` |
| `PlaylistRepository` | CRUD for playlists on LittleFS (JSON files) | `include/live/PlaylistRepository.h` |
| `AppSettingsPersistence` | Load/save settings via NVS (Preferences) | `include/settings/AppSettingsPersistence.h` |
| `StatusJsonBuilder` | Build `/api/status` JSON snapshot of all runtime state | `include/web/StatusJsonBuilder.h` |

## Pattern Overview

**Overall:** Layered, single-threaded Arduino event loop with planner-service pattern

**Key Characteristics:**
- All execution is synchronous within `loop()` — no RTOS tasks, no ISR-heavy logic
- Each domain follows a **Planner → RuntimeService → State** pattern: a stateless planner computes desired state from settings + inputs; a runtime service executes timed actions and produces a state struct
- All external dependencies (Wi-Fi, sensors, NTP, filesystem, GitHub API) go through abstract interfaces (`IWiFiAdapter`, `ISensorSource`, `ITimeSource`, `IFileStore`, `IFirmwareReleaseSource`, `IFirmwareInstaller`)
- Settings flow is unidirectional: `AppSettings` → planner → state struct → consumed by services
- Rendering is double-pass: effect pass (DSL or compiled effect) → overlay pass (clock) → commit to FastLED

## Layers

**Web / API Layer:**
- Purpose: Serve the frontend SPA and expose REST endpoints
- Location: `src/web/`, `include/web/`
- Contains: HTTP route handlers, JSON serialization for each subsystem, embedded asset serving
- Depends on: All runtime services (live, settings, update, presets, playlists)
- Used by: Frontend (`frontend/`) via HTTP; mock backend (`frontend/mockApi.mjs`) in dev mode

**Runtime Services Layer:**
- Purpose: Core business logic — DSL execution, effect rendering, network management, time sync, sensor polling, OTA updates, settings persistence
- Location: `src/live/runtime/`, `src/network/`, `src/time/`, `src/sensors/`, `src/settings/`, `src/update/`, `src/storage/`
- Contains: Stateless services and planners, state structs
- Depends on: Hardware abstraction interfaces, `FrameBuffer`, `MatrixLayout`
- Used by: Web layer (via callbacks), `main.cpp` (loop orchestration)

**DSL Compiler Pipeline:**
- Purpose: Lex → Parse → Compile → Execute user-written effect programs
- Location: `src/live/dsl/` (Lexer, Parser), `src/live/runtime/` (Compiler, Executor)
- Contains: `Lexer` (`include/live/dsl/Lexer.h`), `Parser` (`include/live/dsl/Parser.h`), `Compiler` (`include/live/runtime/Compiler.h`), `Executor` (`include/live/runtime/Executor.h`)
- Depends on: AST types (`include/live/dsl/Ast.h`), `CompiledProgram` bytecode (`include/live/runtime/CompiledProgram.h`)
- Used by: `LiveProgramService`

**Rendering Layer:**
- Purpose: Convert logical pixel state to physical LED output
- Location: `src/FrameBuffer.cpp`, `src/MatrixLayout.cpp`
- Contains: `FrameBuffer` (32×16 logical buffer), `MatrixLayout` (2-panel serpentine mapping)
- Depends on: `AppConfig.h` (panel dimensions, pin assignments)
- Used by: Effect engine, DSL executor, clock overlay

**Hardware Abstraction Layer:**
- Purpose: Decouple business logic from Arduino/ESP32 specifics
- Location: `include/network/IWiFiAdapter.h`, `include/sensors/ISensorSource.h`, `include/time/ITimeSource.h`, `include/storage/IFileStore.h`, `include/update/IFirmwareReleaseSource.h`, `include/update/IFirmwareInstaller.h`
- Contains: Pure virtual interfaces
- Used by: Runtime services (via dependency injection)

**Persistence Layer:**
- Purpose: Store presets, playlists (LittleFS JSON files) and settings (NVS/Preferences)
- Location: `src/storage/`, `src/settings/`
- Contains: `LittleFsFileStore` (`include/storage/LittleFsFileStore.h`), `PreferencesSettingsBackend` (`include/settings/PreferencesSettingsBackend.h`), `AppSettingsPersistence` (`include/settings/AppSettingsPersistence.h`)
- Used by: `PresetRepository`, `PlaylistRepository`, `main.cpp` (settings load/save)

## Data Flow

### Primary Request Path (Live Coding)

1. User submits DSL source via frontend editor → HTTP POST `/api/live/validate` or `/api/live/run` (`src/web/LampWebServer.cpp`)
2. `LiveApi::handleLiveRunRequest()` → `LiveProgramService::runTemporary()` (`include/web/LiveApi.h` → `src/live/runtime/LiveProgramService.cpp`)
3. `LiveProgramService` calls `Compiler::compile()` → `Lexer::tokenize()` → `Parser::parseProgram()` → produces `CompiledProgram`
4. On each `loop()` iteration: `LiveProgramService::render()` → `Executor::render()` → writes pixels to `FrameBuffer`
5. `main.cpp` `renderEffectPass()` → falls through to `EffectRegistry::renderActive()` if no live program active
6. `renderOverlayPass()` → `ClockOverlay::render()` (optional overlay)
7. `commitFrame()` → iterates `FrameBuffer` pixels → `CRGB` array → `FastLED.show()`

### Settings Change Flow

1. Frontend PUT `/api/settings` → `LampWebServer::handleUpdateSettings()` → `saveSettings_` callback
2. `main.cpp` `saveAndApplySettings()` → `AppSettingsPersistence::save()` → `PreferencesSettingsBackend` (NVS)
3. Re-plan: `TimePlanner::plan()` with new settings → update `g_timeState`, `g_runtimeTimeState`
4. If network settings changed: set `g_networkReconfigureRequested = true` → next `loop()` restarts Wi-Fi

### OTA Update Flow

1. Frontend triggers `/api/update/check` → `checkUpdates_` callback → `FirmwareUpdateService::check()` (`src/update/FirmwareUpdateService.cpp`)
2. `ArduinoGitHubReleaseSource::check()` → HTTP GET GitHub Releases API → `GitHubReleaseParser` → `ChecksumFileParser`
3. If version available: frontend shows update button → `/api/update/install` → `FirmwareUpdateService::install()` → `Esp32FirmwareInstaller` → reboot

**State Management:**
- All state is global (`g_` prefix in `main.cpp`), stored as module-level variables in an anonymous namespace
- Planners produce immutable state structs (`PlannedNetworkState`, `PlannedTimeState`, `RuntimeSensorState`, `RuntimeTimeState`)
- `StatusSnapshot` aggregates all state for the web API via `buildStatusSnapshot()`

## Key Abstractions

**Planner-Service-State Pattern:**
- Purpose: Separate "what should happen" (planner) from "make it happen" (service) from "what is happening" (state)
- Examples: `NetworkPlanner` → `WiFiManager` → `PlannedNetworkState`; `TimePlanner` → `TimeRuntimeService` → `RuntimeTimeState`
- Pattern: Planner is a pure function of settings + inputs; Service has side effects (network I/O, NTP); State is a plain struct

**Interface-based Dependency Injection:**
- Purpose: Allow native test builds (`env:native-test`) to substitute mock implementations
- Examples: `IWiFiAdapter` / `ArduinoWiFiAdapter`; `ISensorSource` / `ArduinoAht30SensorSource`; `ITimeSource` / `ArduinoNtpTimeSource`; `IFileStore` / `LittleFsFileStore`; `IFirmwareReleaseSource` / `ArduinoGitHubReleaseSource`; `IFirmwareInstaller` / `Esp32FirmwareInstaller`
- Pattern: Abstract interface in header, Arduino implementation in corresponding `.cpp`, injected at construction

**DSL Pipeline (Lux):**
- Purpose: User-facing effect language compiled to bytecode for execution on ESP32-C3
- Examples: `include/live/dsl/Ast.h`, `include/live/dsl/Lexer.h`, `include/live/dsl/Parser.h`, `include/live/runtime/Compiler.h`, `include/live/runtime/Executor.h`, `include/live/runtime/CompiledProgram.h`
- Pattern: Lexer (tokens) → Parser (AST `Program`) → Compiler (bytecode `CompiledProgram` with expression trees) → Executor (pixel-level evaluation per frame)

## Entry Points

**Firmware Entry:**
- Location: `src/main.cpp`
- Triggers: ESP32 boot → `setup()` → `loop()`
- Responsibilities: Initialize all globals, mount LittleFS, start Wi-Fi, begin web server, run render/overlay/commit loop, poll sensors/time/playlist at intervals

**Frontend Entry:**
- Location: `frontend/src/main.ts`
- Triggers: Browser loads `index.html` served from ESP32 (production) or Vite dev server (development)
- Responsibilities: Initialize editor UI, connect to lamp API, manage live coding workflow, settings panels, OTA UI

**Mock Backend Entry:**
- Location: `frontend/mockApi.mjs`
- Triggers: Vite dev server plugin intercepts `/api/*` requests
- Responsibilities: Simulate lamp API responses for frontend development without hardware

**Build System Entry:**
- Location: `platformio.ini`
- Triggers: `pio run` commands
- Responsibilities: Define 3 environments (dev, release, native-test), pre-build scripts (`generate_version.py`, `embed_resources.py`), dependency management

**Pre-Build Scripts:**
- `scripts/generate_version.py`: Generates version string from git tags/commits
- `scripts/embed_resources.py`: Compresses `resources/dist/` (frontend build output) into `include/embedded_resources.h` as gzipped PROGMEM byte arrays

## Architectural Constraints

- **Threading:** Single-threaded Arduino event loop. No RTOS tasks, no interrupts beyond Arduino framework defaults. All operations must be non-blocking or short-lived.
- **Global state:** All runtime objects are global in `src/main.cpp` anonymous namespace (`g_` prefix). No heap allocation after setup. No dynamic service registration.
- **Circular imports:** Not detected. Headers use forward declarations and `#pragma once`.
- **Memory:** ESP32-C3 constrained (400KB SRAM, 4MB Flash). Frontend assets are gzip-compressed and stored as `PROGMEM` byte arrays. FrameBuffer is a `std::vector<Rgb>` of 512 elements.
- **Flash layout:** OTA-capable partition scheme (`partitions_ota.csv`) with two app slots and a factory partition.

## Anti-Patterns

### Global Service Locator via Callbacks

**What happens:** `LampWebServer` receives services via setter methods (`setPresetServices()`, `setPlaylistServices()`, `setUpdateCallbacks()`, `setSettingsCallbacks()`), creating an implicit service locator pattern.
**Why it's wrong:** Dependencies are not explicit in constructor; order of `set*` calls matters; null pointer risks if a setter is missed.
**Do this instead:** Pass all dependencies in `LampWebServer` constructor or use a dedicated context struct. The current pattern works for the scale of this project but would become fragile with more services.

### Global Mutable State Without Encapsulation

**What happens:** All runtime state lives in global variables in `src/main.cpp` (`g_` prefix), accessed directly by `buildStatusSnapshot()` and various loop-phase functions.
**Why it's wrong:** No single source of truth; any function in `main.cpp` can mutate any global; testing individual functions requires mocking all globals.
**Do this instead:** Encapsulate related state+behavior into a `LampRuntime` class that owns all services and state. `main.cpp` should only instantiate and call `setup()`/`loop()` on it.

## Error Handling

**Strategy:** Return values (bool, error strings) propagated up the call stack. No exceptions (Arduino/freestanding environment).

**Patterns:**
- Services return `bool` for success/failure, with an output `std::string& error` parameter
- `LiveProgramService` returns diagnostics vectors (`std::vector<Diagnostic>`) for user-facing DSL errors with line/column positions
- `FirmwareUpdateService::status()` exposes `FirmwareUpdateStatus` with `state` enum and `error` string
- Sensor service tracks `consecutiveMisses` and sets `available = false` after `kSensorStaleReadLimit` failures

## Cross-Cutting Concerns

**Logging:** `Serial.print`/`Serial.println` — no structured logging framework. Used in `setup()` for boot banner and in error paths.
**Validation:** DSL programs validated through full lex→parse→compile pipeline, returning line-numbered diagnostics. Settings validated at persistence boundary (normalization functions in `AppSettingsPersistence`).
**Authentication:** No authentication. Device runs its own Wi-Fi AP or joins a configured network. Web server is open on LAN.
**Build Identity:** Version, channel, board, hardware type injected via `-D` build flags → `BuildInfo.h` → `BuildIdentity` struct. Channel determines OTA update source filtering.

---

*Architecture analysis: 2026-06-02*
