# Testing Patterns

**Analysis Date:** 2026-06-02

## Test Framework

**C++ Runner:**
- PlatformIO Unity Test Framework (`test_framework = unity`)
- Config: `platformio.ini` → `[env:native-test]` environment
- Platform: `native` (tests run on host, not on ESP32)

**Run Commands:**
```bash
pio test -e native-test                    # Run all C++ tests
pio test -e native-test -f test_framebuffer  # Run single test module
pio test -e native-test --verbose           # Verbose output
```

**Frontend Runner:**
- Node.js built-in `assert` module (no test framework)
- Config: `frontend/package.json` → `"test"` script

**Frontend Run Commands:**
```bash
cd frontend
npm run test              # Run both layout and mock-api tests
npm run test:layout       # Run layout template test only
npm run test:mock         # Run mock API server test only
```

**Assertion Library (C++):**
- Unity assertions: `TEST_ASSERT_EQUAL_UINT8`, `TEST_ASSERT_EQUAL_UINT32`, `TEST_ASSERT_EQUAL_STRING`, `TEST_ASSERT_EQUAL_INT`, `TEST_ASSERT_TRUE`, `TEST_ASSERT_FALSE`, `TEST_ASSERT_NOT_EQUAL`, `TEST_ASSERT_LESS_OR_EQUAL_UINT32_MESSAGE`, `TEST_FAIL_MESSAGE`

**Assertion Library (Frontend):**
- Node.js `assert` module: `assert.equal`, `assert.match`, `assert.ok`

## Test File Organization

**Location:**
- C++ tests: `test/test_<module>/test_main.cpp` (one directory per module, each with a single `test_main.cpp`)
- Frontend tests: `frontend/test/<name>.test.mjs` (flat directory)

**Naming:**
- Test directories: `test_<module_name>` with `snake_case` (e.g., `test_framebuffer`, `test_dsl_executor`, `test_effect_registry`, `test_playlist_repository`, `test_build_identity`, `test_live_request_json`, `test_network_settings_api`, `test_settings_persistence`, `test_time_policy`)
- Test functions: `test_<descriptive_snake_case>` (e.g., `test_set_pixel_uses_wrapped_x_coordinates`, `test_clear_resets_all_pixels_to_black`, `test_registry_switches_active_effect_by_name`)
- Frontend test files: `<name>.test.mjs` (e.g., `layout.test.mjs`, `mock-api.test.mjs`)

**Structure:**
```
test/
├── test_framebuffer/
│   └── test_main.cpp
├── test_effect_registry/
│   └── test_main.cpp
├── test_dsl_executor/
│   └── test_main.cpp
├── test_playlist_repository/
│   └── test_main.cpp
├── test_playlist_scheduler/
│   └── test_main.cpp
├── test_build_identity/
│   └── test_main.cpp
├── test_checksum_file_parser/
│   └── test_main.cpp
├── test_dev_version_matching/
│   └── test_main.cpp
├── test_dsl_parser/
│   └── test_main.cpp
├── test_effects/
│   └── test_main.cpp
├── test_embedded_asset_metadata/
│   └── test_main.cpp
├── test_file_store/
│   └── test_main.cpp
├── test_firmware_update_service/
│   └── test_main.cpp
├── test_github_release_parser/
│   └── test_main.cpp
├── test_live_api/
│   └── test_main.cpp
├── test_live_program_service/
│   └── test_main.cpp
├── test_live_request_json/
│   └── test_main.cpp
├── test_network_modes/
│   └── test_main.cpp
├── test_network_settings_api/
│   └── test_main.cpp
├── test_pattern_effects/
│   └── test_main.cpp
├── test_playlist_api/
│   └── test_main.cpp
├── test_playlist_json/
│   └── test_main.cpp
├── test_preset_api/
│   └── test_main.cpp
├── test_preset_json/
│   └── test_main.cpp
├── test_preset_repository/
│   └── test_main.cpp
├── test_sensor_runtime/
│   └── test_main.cpp
├── test_settings_persistence/
│   └── test_main.cpp
├── test_status_json/
│   └── test_main.cpp
├── test_time_policy/
│   └── test_main.cpp
├── test_time_runtime/
│   └── test_main.cpp
├── test_time_settings_api/
│   └── test_main.cpp
└── test_wifi_manager/
    └── test_main.cpp
```

## Test Structure

**Suite Organization (C++):**
```cpp
#include <unity.h>

#include "ModuleUnderTest.h"

namespace {

void test_case_one() {
  // Arrange
  // Act
  // Assert
}

void test_case_two() {
  // Arrange
  // Act
  // Assert
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_case_one);
  RUN_TEST(test_case_two);
  return UNITY_END();
}
```

**Patterns:**
- All test functions in anonymous `namespace {}` to avoid linker collisions
- Each test file has exactly one `main()` entry point
- `(void)argc; (void)argv;` to suppress unused parameter warnings
- No `setUp()` or `tearDown()` — each test creates its own state inline (Arrange-Act-Assert)
- Tests are independent; no shared state between test functions
- Assertions use `TEST_ASSERT_*` macros exclusively

**Frontend Test Structure:**
```javascript
import assert from "assert";

async function main() {
  // Arrange: set up test state
  // Act: call function under test
  // Assert: assert.* calls
}

main().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
```

## Mocking

**Framework:** Manual mock implementations (no mocking library)
- Mock interfaces are defined as local classes in test files
- They implement the same abstract interface as production code

**C++ Mock Patterns:**
```cpp
// In-memory stub implementing the interface
class MemoryFileStore : public lamp::storage::IFileStore {
 public:
  bool isReady() const override { return true; }
  bool writeText(const std::string& path, const std::string& content) override {
    // In-memory implementation
    entries_.push_back({path, content});
    return true;
  }
  // ... other overrides
 private:
  struct Entry { std::string path; std::string content; };
  std::vector<Entry> entries_;
};

// Spy that counts calls for verification
class UnreadyFileStore : public lamp::storage::IFileStore {
 public:
  bool isReady() const override { return false; }
  // All ops return false and increment counters
  mutable int readCalls = 0;
  int writeCalls = 0;
  // ...
};

// Fake with configurable behavior
class FakeSettingsBackend final : public lamp::settings::ISettingsBackend {
 public:
  bool ready = true;
  std::map<std::string, std::string> values;
  // ... implementations reading from values map
};
```

**What to Mock:**
- Storage backends (`IFileStore` → `MemoryFileStore`, `UnreadyFileStore`)
- Settings backends (`ISettingsBackend` → `FakeSettingsBackend`)
- External adapters not available in native tests

**What NOT to Mock:**
- Domain objects (`FrameBuffer`, `MatrixLayout`, `EffectRegistry`)
- DSL parsers/compilers/executors (tested end-to-end together)
- Value objects (structs like `Rgb`, `PresetModel`, `Diagnostic`)

**Frontend Mock Pattern:**
- `frontend/mockApi.mjs` provides a full HTTP mock server for local development
- `frontend/test/mock-api.test.mjs` tests the mock server itself by spinning up an HTTP server and making real requests
- Mock scenarios are defined in `frontend/src/dev/mockScenarios.ts` and selected via `X-Dev-Scenario` header

## Fixtures and Factories

**Test Data (C++):**
```cpp
// Factory functions for test models
lamp::live::PlaylistModel makePlaylist(const char* id, const char* name, bool repeat,
                                       uint32_t firstDurationSec) {
  lamp::live::PlaylistModel playlist;
  playlist.id = id;
  playlist.name = name;
  playlist.repeat = repeat;
  playlist.entries.push_back({"warm_waves", firstDurationSec, true});
  return playlist;
}

// Source code builders for DSL tests
std::string makeSourceForColor(uint32_t red, uint32_t green, uint32_t blue) {
  return std::string("effect \"dot\"\n") +
         "sprite dot {\n"
         "  bitmap \"\"\"\n"
         "  #\n"
         "  \"\"\"\n"
         "}\n"
         "layer dot1 {\n"
         "  use dot\n"
         "  color rgb(" + std::to_string(red) + ", " + ... + ")\n"
         // ...
         "}\n";
}
```

**Frontend Fixture Pattern:**
- Predefined snippet sources in `mockApi.mjs` (e.g., `snippetSources["warm-waves"]`, `snippetSources["clock"]`)
- Scenarios defined in `frontend/src/dev/mockScenarios.ts` with full initial state per scenario

**Location:**
- C++: factory functions in anonymous namespace within each `test_main.cpp`
- Frontend: mock state in `frontend/src/dev/mockScenarios.ts`, mock server in `frontend/mockApi.mjs`

## Coverage

**Requirements:** No coverage target enforced
- No coverage configuration detected in `platformio.ini` or `frontend/package.json`

## Test Types

**Unit Tests:**
- C++ tests under `test/` are unit tests — each tests a single module/class
- Frontend layout test verifies HTML template output
- Frontend mock API test verifies mock server HTTP behavior
- All tests run on host (native platform) — no hardware-in-the-loop tests

**Integration Tests:**
- DSL executor tests (`test_dsl_executor`) are integration-level: they exercise Lexer→Parser→Compiler→Executor pipeline end-to-end
- Playlist scheduler tests (`test_playlist_scheduler`) exercise `LiveProgramService` + `PlaylistRepository` + `PlaylistScheduler` together

**E2E Tests:**
- Not used
- Manual testing via `pio run -t upload` + browser UI

## Common Patterns

**Async Testing (Frontend):**
```javascript
async function main() {
  const { server, baseUrl } = await startMockServer();
  try {
    const response = await requestJson(baseUrl, "/api/update/current");
    assert.equal(response.statusCode, 200);
  } finally {
    await new Promise((resolve) => server.close(resolve));
  }
}
```

**Error Testing (C++):**
```cpp
void test_registry_rejects_unknown_effect_names() {
  lamp::effects::EffectRegistry registry;
  lamp::effects::SolidColorEffect first(lamp::Rgb{1, 2, 3}, "solid-color-1");
  registry.add(first);
  TEST_ASSERT_FALSE(registry.setActiveByName("missing"));
}

void test_apply_network_settings_update_rejects_unknown_mode() {
  lamp::settings::AppSettings settings;
  const bool applied = lamp::web::applyNetworkSettingsUpdate(
      "invalid", "MYLAMP-ROOM", "HomeWiFi", "secret", settings);
  TEST_ASSERT_FALSE(applied);
  // Verify settings were NOT modified
  TEST_ASSERT_EQUAL_INT(static_cast<int>(lamp::network::NetworkMode::kAccessPoint),
                        static_cast<int>(settings.network.preferredMode));
}
```

**State Verification Testing:**
```cpp
void test_load_uses_defaults_when_storage_is_empty() {
  FakeSettingsBackend backend;
  lamp::settings::AppSettingsPersistence persistence;
  const lamp::settings::AppSettings settings = persistence.load(backend);
  TEST_ASSERT_EQUAL_STRING("MYLAMP", settings.network.accessPointName.c_str());
  TEST_ASSERT_EQUAL_STRING("UTC0", settings.clock.timezone.c_str());
}
```

**Planner/Pure Logic Testing:**
```cpp
void test_client_mode_with_internet_enables_ntp_sync() {
  lamp::settings::AppSettings settings;
  lamp::network::PlannedNetworkState networkState;
  networkState.activeMode = lamp::network::NetworkMode::kClient;
  networkState.timeSyncAllowed = true;
  lamp::time::TimePlanner planner;
  const lamp::time::PlannedTimeState timeState =
      planner.plan(settings.clock, networkState, false);
  TEST_ASSERT_TRUE(timeState.ntpSyncEnabled);
  TEST_ASSERT_EQUAL_STRING("Clock: NTP", timeState.statusLine.c_str());
}
```

## Test Build Configuration

**C++ Test Build (`platformio.ini` `[env:native-test]`):**
- Platform: `native` (tests compile and run on host, not ESP32)
- Only selected source files are compiled via `build_src_filter` (whitelist with `+<...>`)
- `main.cpp` is excluded (`-<main.cpp>`) — each test provides its own `main()`
- Common library dependencies (ArduinoJson) included
- Build flags define test-specific macros (e.g., `-D APP_VERSION=\"0.1.0-test\"`)

**Frontend Test Build:**
- Layout test: TypeScript compiled to JS first (`tsc --outDir .tmp-tests`), then run with Node
- Mock API test: runs mock server inline using `http` module, no compilation needed (`.mjs` ES modules)

## Test Coverage by Module

| Module | Test Directory | What's Tested |
|--------|---------------|---------------|
| FrameBuffer | `test_framebuffer` | Pixel wrapping, clear, set/get |
| EffectRegistry | `test_effect_registry` | Add, switch by name, reject unknown |
| DSL Parser | `test_dsl_parser` | Lex/parse DSL source |
| DSL Executor | `test_dsl_executor` | End-to-end DSL→rendering, animation, layer order |
| Effect implementations | `test_effects`, `test_pattern_effects` | SolidColor, AlternatingColumns, ClockOverlay |
| PresetRepository | `test_preset_repository` | CRUD, file store integration |
| Preset JSON | `test_preset_json` | JSON serialization/deserialization |
| Preset API | `test_preset_api` | HTTP request handling |
| PlaylistRepository | `test_playlist_repository` | CRUD, not-ready guard |
| Playlist JSON | `test_playlist_json` | JSON serialization/deserialization |
| Playlist API | `test_playlist_api` | HTTP request handling |
| PlaylistScheduler | `test_playlist_scheduler` | Autoplay, entry transitions |
| LiveRequest JSON | `test_live_request_json` | JSON parse, diagnostic response format |
| Live API | `test_live_api` | Validate, run endpoints |
| LiveProgramService | `test_live_program_service` | Compile, activate, render |
| Network settings | `test_network_modes`, `test_network_settings_api`, `test_wifi_manager` | Mode parsing, settings JSON, WiFi startup |
| Time settings | `test_time_policy`, `test_time_runtime`, `test_time_settings_api` | Planner logic, runtime refresh, JSON API |
| Sensor runtime | `test_sensor_runtime` | Sensor refresh, stale detection |
| Settings persistence | `test_settings_persistence` | Load/save, NVS key limits |
| Status JSON | `test_status_json` | Status payload formatting |
| Build identity | `test_build_identity` | Compile-time macro verification |
| Checksum file parser | `test_checksum_file_parser` | SHA256 checksum file parsing |
| Dev version matching | `test_dev_version_matching` | Version comparison logic |
| GitHub release parser | `test_github_release_parser` | Release JSON parsing |
| Firmware update service | `test_firmware_update_service` | Update state machine |
| Embedded asset metadata | `test_embedded_asset_metadata` | Gzip content type detection |
| File store | `test_file_store` | LittleFS file I/O |
| Frontend layout | `frontend/test/layout.test.mjs` | HTML template rendering |
| Frontend mock API | `frontend/test/mock-api.test.mjs` | Mock server HTTP behavior |

---

*Testing analysis: 2026-06-02*
