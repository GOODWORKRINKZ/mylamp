# Codebase Concerns

**Analysis Date:** 2026-06-02

## Security Considerations

### Hardcoded AP Password

- Risk: Default Wi-Fi access point password `12345678` is compiled into firmware and identical across all devices. Anyone within Wi-Fi range who knows this default can connect and control the lamp.
- Files: `include/AppConfig.h:21`
- Current mitigation: None. Password is a compile-time constant.
- Recommendations: Generate a unique per-device password derived from the ESP32 MAC address, or require the user to set a password on first boot through a captive portal.

### Wi-Fi Client Password Stored in Plaintext

- Risk: The user's Wi-Fi network password is stored as a plaintext string in ESP32 NVS (Preferences). Any code running on the device can read it, and it persists across firmware updates in the NVS partition.
- Files: `src/settings/AppSettingsPersistence.cpp:10` (`kNetworkClientPasswordKey`), saved via `PreferencesSettingsBackend`
- Current mitigation: None. NVS is not encrypted on ESP32-C3 without enabling flash encryption.
- Recommendations: Enable ESP32 flash encryption in production builds, or use the ESP32 HMAC peripheral to derive a key for encrypting sensitive NVS entries.

### No Authentication on Web API

- Risk: All REST API endpoints (`/api/presets`, `/api/live/run`, `/api/update/install`, `/api/settings/network`, etc.) are accessible without any authentication. Any device on the same local network can read Wi-Fi credentials, modify presets, trigger OTA firmware updates, or change device settings.
- Files: `src/web/LampWebServer.cpp` — all `handle*()` methods; `include/web/LampWebServer.h:58` — `server_(80)`
- Current mitigation: Relies on Wi-Fi network isolation only.
- Recommendations: Add a simple shared-secret token (stored in NVS) required in an `Authorization` header for all mutating endpoints. Consider a PIN-based pairing flow for initial trust establishment.

### Manual JSON String Building (Injection Risk)

- Risk: Several API response builders construct JSON via manual string concatenation (`std::string +=`) with custom escaping. While `escapeJsonValue` is used, this pattern is error-prone compared to a proper JSON library. A missed escape on user-controlled data (preset names, effect source, SSID names) could produce malformed or malicious JSON responses.
- Files: `src/web/LampWebServer.cpp:78-111` (`buildUpdateSettingsJson`, `buildCurrentUpdateJson`, `buildCheckUpdatesJson`), `src/web/NetworkSettingsJson.cpp:36-43` (`buildNetworkSettingsJson`)
- Current mitigation: `escapeJsonValue()`/`escapeJson()` functions escape `\`, `"`, `\n`, `\r`, `\t` — but this is a subset; Unicode escapes and other control characters are not handled.
- Recommendations: Migrate all manual JSON builders to `ArduinoJson` (already a project dependency at `^6.21.5`). The `StatusJsonBuilder` already uses ArduinoJson; extend this pattern to all API responses.

### Password Transmitted as URL Query Parameter

- Risk: Wi-Fi client password is sent via `POST /api/settings/network` with `clientPassword` as a form/query parameter. While HTTPS would protect this in transit, the device uses plain HTTP. The password appears in URL-encoded form data which may be logged by intermediate proxies (if any).
- Files: `src/web/LampWebServer.cpp:234` (`server_.arg("clientPassword")`)
- Current mitigation: None.
- Recommendations: Send sensitive data in a JSON request body instead. Add HTTPS support for the web interface, or at minimum document this as a local-network-only risk.

## Tech Debt

### Global Mutable State in main.cpp

- Issue: Over 40 global variables in an anonymous namespace — singletons, runtime state, timing trackers, flags, and error strings. This makes the system hard to test in isolation and creates implicit coupling between unrelated subsystems.
- Files: `src/main.cpp:44-95` (all `g_*` variables)
- Impact: Any change to the global state model requires understanding all 40+ variables and their interactions. Refactoring individual subsystems is risky because the globals are passed by reference throughout the codebase.
- Fix approach: Introduce a `LampApplication` or `DeviceRuntime` struct that owns all subsystem instances. Pass it explicitly rather than relying on globals. Phase this in one subsystem at a time (e.g., start with sensor state, then time, then network).

### Large Monolithic Frontend File

- Issue: `main.ts` is 2,197 lines — it contains all UI logic (network modal, OTA flow, preset/playlist manager, editor, status bar, event handlers) in a single file with no module decomposition.
- Files: `frontend/src/main.ts` (2197 lines)
- Impact: Difficult to navigate, test, or modify without unintended side effects. Merge conflicts are likely when multiple features are worked on simultaneously.
- Fix approach: Split into feature modules: `presets.ts`, `playlists.ts`, `network.ts`, `ota.ts`, `editor.ts`, `status.ts`. The `shellTemplate.ts` already shows the correct pattern — extend this.

### Manual JSON in Web Server Response Builders

- Issue: `LampWebServer.cpp` builds JSON responses for update settings, current update info, and check-updates results using manual string concatenation, while `ArduinoJson` (a dependency) is already used by `StatusJsonBuilder` and `PresetApi`.
- Files: `src/web/LampWebServer.cpp:78-111`
- Impact: Inconsistent JSON generation patterns. Manual escaping is fragile and incomplete. Adding new fields requires careful string manipulation.
- Fix approach: Replace `buildUpdateSettingsJson`, `buildCurrentUpdateJson`, `buildCheckUpdatesJson` with `ArduinoJson`-based builders, following the `StatusJsonBuilder` pattern.

### Heartbeat Debug Artifact in Production

- Issue: Every 5 seconds, when no live DSL program is active, the firmware alternates between a "boot-solid" effect and a "debug-columns" pattern effect. This is clearly a development heartbeat/visual indicator, not intended for end-user experience.
- Files: `src/main.cpp:370-392`
- Impact: End users see a flickering debug pattern when no effect is running. Degrades the product experience.
- Fix approach: Remove the heartbeat alternation or gate it behind a build flag (`#ifdef APP_CHANNEL == "dev"`). Let the boot-solid effect be the stable idle state in release builds.

### DSL Recursive Expression Evaluation

- Issue: `Executor::evaluateNode` is a recursive function with no depth limit. Deeply nested expressions (e.g., `sin(sin(sin(...)))` chained many times) could overflow the ESP32-C3 stack.
- Files: `src/live/runtime/Executor.cpp:32-105`
- Impact: Stack overflow on malicious or accidentally-deep DSL expressions could crash the device.
- Fix approach: Add a recursion depth counter with a hard limit (e.g., 64 levels). Fail compilation or execution if exceeded.

## Performance Bottlenecks

### OTA Download Blocking with No Timeout

- Problem: The firmware download loop in `Esp32FirmwareInstaller::install` uses a busy-wait with `delay(1)` and no timeout. If the HTTP connection stalls mid-download, the device hangs indefinitely in the update loop, blocking all other operations including the watchdog.
- Files: `src/update/Esp32FirmwareInstaller.cpp:112-138`
- Cause: The `while (http.connected() ...)` loop only checks `stream->available()`. If the server stops sending data but keeps the connection open, the loop never exits.
- Improvement path: Add a configurable download timeout (e.g., 120 seconds). Track `millis()` at the start and abort if the total elapsed time exceeds the limit. Also add a stall timeout — if no bytes are received for N seconds, abort.

### Manual String Concatenation in JSON Builders

- Problem: Multiple `std::string +=` operations in JSON response builders cause repeated heap allocations.
- Files: `src/web/LampWebServer.cpp:78-111`, `src/web/NetworkSettingsJson.cpp:36-43`
- Cause: Each `+=` may trigger a reallocation and copy of the entire string.
- Improvement path: Use `ArduinoJson` with pre-allocated `StaticJsonDocument` or `DynamicJsonDocument` with appropriate capacity. Reserve string capacity before building.

### No Free Flash Space Check Before OTA

- Problem: `Update.begin()` is called with `UPDATE_SIZE_UNKNOWN` when `contentLength <= 0`. This means the OTA process starts without verifying there is enough free space in the target partition.
- Files: `src/update/Esp32FirmwareInstaller.cpp:104`
- Cause: HTTP response may not include `Content-Length` header.
- Improvement path: Check `Update.isRunning()` partition size vs. `contentLength` when available. When unknown, check free space in the OTA partition via `ESP.getFreeSketchSpace()` before beginning.

## Fragile Areas

### EffectRegistry Raw Pointer Ownership

- Files: `include/effects/EffectRegistry.h:20` (`std::vector<IEffect*> effects_`), `src/effects/EffectRegistry.cpp`
- Why fragile: Effects are registered by raw pointer from `main.cpp` globals. If an effect object is destroyed before the registry (e.g., during a refactor), `renderActive()` will dereference a dangling pointer. The registry has no way to detect this.
- Safe modification: Always ensure effects outlive the registry. Consider using `std::vector<std::reference_wrapper<IEffect>>` (clearer intent) or a `shared_ptr` model.
- Test coverage: `test/test_effect_registry/` exists but only tests basic add/setActive/render patterns — does not test lifecycle safety.

### Web Server Route Dispatching via URL Prefix Matching

- Files: `src/web/LampWebServer.cpp:179-190` (`handleNotFound` routes `/api/presets/` and `/api/playlists/` by string prefix)
- Why fragile: URL dispatch depends on `startsWith` matching and manual path trimming. Adding a new nested route (e.g., `/api/presets/export`) requires careful ordering. The `handlePresetByPath` method is 70+ lines of manual suffix parsing with string comparison.
- Safe modification: When adding new preset/playlist endpoints, ensure they are registered before the catch-all `onNotFound` handler or use more specific `server.on()` patterns.
- Test coverage: `test/test_preset_api/` and `test/test_playlist_api/` cover the main flows but not edge cases like double-slashes or URL-encoded IDs.

### LittleFS File Store — No Write Failure Recovery

- Files: `src/storage/LittleFsFileStore.cpp:31-44` (`writeText`)
- Why fragile: `writeText` opens a file, writes, and closes. If the write partially succeeds (power loss mid-write), the file is left in a corrupted state. There is no atomic write-then-rename pattern, and no checksum verification on read.
- Safe modification: For critical data (presets, playlists), write to a temp file first, then rename. Add a CRC or length prefix on read to detect corruption.
- Test coverage: `test/test_file_store/` exists (175 lines) but primarily tests happy-path read/write/list/remove. Power-loss simulation is not tested.

### CompiledProgram/LiveProgramService State Lifecycle

- Files: `src/live/runtime/LiveProgramService.cpp:15-47` (`runTemporary`, `activatePreset`, `stop`)
- Why fragile: `CompiledProgram` contains `std::vector<ExpressionNode>` and `std::vector<CompiledLayer>` — these are deep-copied on assignment. If the DSL program is large (many sprites, many layers), the copy could be expensive. Additionally, `state_` and `activeProgram_` are mutated by both the web server (in request context) and the main loop (for rendering), though the ESP32 WebServer is single-threaded per request.
- Safe modification: Consider move semantics for `CompiledProgram` assignments. Ensure `render()` is never called concurrently with `runTemporary()`.
- Test coverage: `test/test_live_program_service/` covers temporary/activate/stop, but not large programs.

## Scaling Limits

### ESP32-C3 Memory Constraints

- Current capacity: ~400 KB SRAM on ESP32-C3. `CompiledProgram` stores all expression nodes, compiled sprites (each with pixel arrays), and compiled layers in RAM.
- Limit: Large DSL programs with many sprites (e.g., full-font text sprites with hundreds of glyphs) could exhaust heap. There is no program-size limit enforced at compile time.
- Scaling path: Add a configurable limit on total expression nodes, sprite pixels, and layer count. Validate at compile time and reject programs that exceed limits with a clear diagnostic.

### OTA Partition Size

- Current capacity: Defined by `partitions_ota.csv` (not inspected for exact sizes — contains partition table).
- Limit: Firmware binary must fit within the OTA partition. As the codebase grows (more effects, larger web assets embedded), it will approach the partition limit.
- Scaling path: Monitor firmware size in CI. Consider splitting rarely-used features into optional components. Compress embedded web assets (already using gzip Content-Encoding in `sendEmbeddedAsset`).

### Single HTTP Connection at a Time

- Current capacity: ESP32 WebServer library handles one request at a time (single-threaded event loop).
- Limit: No concurrent API requests. A slow OTA download (HTTP GET for firmware binary) blocks all other API interactions for the duration.
- Scaling path: Offload OTA downloads to a separate task on the FreeRTOS second core, or use async HTTP patterns. Acceptable for current single-user usage but limits future multi-user scenarios.

## Dependencies at Risk

### Vite 2.9.18 and TypeScript 4.9.5

- Risk: Vite 2.x reached EOL in 2023. TypeScript 4.9.5 is from January 2023. No security patches are being backported to these versions.
- Files: `frontend/package.json:14-15`
- Impact: Potential unpatched vulnerabilities in the dev server. New language features and type improvements are unavailable.
- Migration plan: Upgrade to Vite 5.x or 6.x (current stable). Upgrade TypeScript to 5.x. Test the Vite dev server and build output after migration.

### ArduinoJson 6.x

- Risk: ArduinoJson 6.x is mature and stable, but version 7.x has been released with API changes. Staying on 6.x means missing performance improvements and potential future compatibility issues with newer ESP32 Arduino core versions.
- Files: `platformio.ini:13` (`bblanchon/ArduinoJson @ ^6.21.5`)
- Impact: Low immediate risk. The library is well-maintained and v6 continues to receive fixes.
- Migration plan: Plan migration to ArduinoJson 7.x as a separate phase. The API changes are well-documented.

## Test Coverage Gaps

### Frontend Main Logic

- What's not tested: The 2,197-line `main.ts` has no unit tests for its core logic — preset list rendering, playlist management, OTA reboot/retry flow, network settings form validation, editor status sync, or status bar updates.
- Files: `frontend/src/main.ts` (2,197 lines)
- Risk: UI regressions are caught only by manual testing. The OTA reboot retry loop (5 attempts with polling) is particularly risky to change without test coverage.
- Priority: High. Add tests for: preset CRUD flow, playlist start/stop, OTA retry logic, network form validation, and status snapshot rendering.

### OTA Installer Integration

- What's not tested: The `Esp32FirmwareInstaller` is not tested in isolation (no `test_esp32_firmware_installer/` directory). The `FirmwareUpdateService` test only covers the state machine, not the actual install path.
- Files: `src/update/Esp32FirmwareInstaller.cpp` (170 lines) — no corresponding test directory
- Risk: Changes to checksum verification, download streaming, or error handling in the installer cannot be validated without a physical device.
- Priority: Medium. Mock the HTTP and Update APIs for unit testing. Test checksum mismatch, partial download, and WiFi disconnect scenarios.

### Compiler Edge Cases

- What's not tested: Deeply nested expressions, maximum argument counts, Unicode in sprite bitmaps, very long effect names, and interaction between `text` sprites and `sprite` sprites.
- Files: `src/live/runtime/Compiler.cpp` (506 lines), tested indirectly via `test/test_dsl_executor/` (464 lines)
- Risk: DSL compilation may have undiscovered edge cases that pass parsing and compilation but produce incorrect rendered output.
- Priority: Medium. The existing DSL executor tests are thorough for the happy path. Add fuzz-style tests with random valid DSL programs.

### File System Corruption Recovery

- What's not tested: Power-loss during preset/playlist write, filesystem full conditions, directory creation failures, or corrupted file reads.
- Files: `src/storage/LittleFsFileStore.cpp`, tested in `test/test_file_store/` (175 lines)
- Risk: Silent data loss on unexpected power-off. No recovery mechanism exists.
- Priority: Medium. Add tests for write-then-read consistency with simulated partial writes. Consider adding atomic write pattern.

## Missing Critical Features

### Production-Grade OTA Rollback

- Problem: The current OTA flow installs firmware and reboots. If the new firmware fails to boot (e.g., crashes during initialization), the device is bricked with no automatic rollback to the previous working version.
- Blocks: Safe automatic updates in production. Currently acknowledged in `docs/STATUS.md` as "not fully closed end-to-end."
- Files: `src/update/Esp32FirmwareInstaller.cpp`, `src/main.cpp` (no boot-validation logic)
- Recommendation: Implement ESP32 OTA rollback using the built-in `esp_ota_get_next_update_partition` / `esp_ota_set_boot_partition` mechanism. Add a boot-validation flag that gets cleared only after the device successfully runs for N seconds post-update.

### No Device Health/Telemetry Endpoint

- Problem: There is no API endpoint to query device health metrics — free heap, flash usage, uptime, WiFi RSSI, last crash reason, or boot count.
- Blocks: Remote debugging of device issues. Proactive monitoring.
- Recommendation: Add a `GET /api/health` endpoint returning heap free, sketch size, free sketch space, uptime seconds, last reset reason, and WiFi signal strength.

### No Configuration Backup/Restore

- Problem: All settings (Wi-Fi, timezone, presets, playlists) live only in NVS flash. There is no way to export or import configuration for backup or device migration.
- Blocks: Smooth device replacement. Bulk configuration.
- Recommendation: Add `GET /api/settings/export` and `POST /api/settings/import` endpoints that serialize/deserialize all settings and content.

---

*Concerns audit: 2026-06-02*
