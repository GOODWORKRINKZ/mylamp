# Microbbox-Style OTA Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Добавить в `mylamp` firmware OTA и release pipeline по образцу `microbbox`, с GitHub Releases, каналами `stable/dev` и архитектурой под несколько hardware targets.

**Architecture:** Реализация идет от build identity и release metadata к OTA runtime, затем к web/API/UI и GitHub Actions. Внешний release contract сразу делается multi-hardware, но пока активен один target. OTA install остается ручным, check/install идут через отдельный firmware update service и встроенный web API.

**Tech Stack:** PlatformIO, Arduino/C++, ESP32 OTA APIs, LittleFS/Preferences, TypeScript/Vite frontend, GitHub Actions, Python helper script, shell release helpers.

---

### Task 1: Build identity and version generation

**Files:**
- Create: `scripts/generate_version.py`
- Modify: `platformio.ini`
- Modify: `include/BuildInfo.h`
- Modify: `scripts/embed_resources.py`
- Test: `test/test_status_json/test_main.cpp`

**Step 1: Write the failing tests for new build identity fields**

Add assertions in `test/test_status_json/test_main.cpp` for future fields such as `hardwareType` and update status placeholders in the JSON.

```cpp
TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(json.find("\"hardwareType\":\"c3-cylinder32x16\"")));
TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(json.find("\"updateChannel\":\"dev\"")));
```

**Step 2: Run the focused test and verify it fails**

Run: `cd /home/builder/mylamp && /home/builder/.platformio/penv/bin/pio test -e native-test -f test_status_json`

Expected: FAIL because `StatusSnapshot` and `BuildInfo` do not contain the new OTA-related identity fields yet.

**Step 3: Add compile-time identity support**

Update `include/BuildInfo.h` so it exposes:

```cpp
#ifndef APP_HARDWARE_TYPE
#define APP_HARDWARE_TYPE "unknown-hardware"
#endif

struct BuildInfo {
  static constexpr const char* projectName = "mylamp";
  static constexpr const char* version = APP_VERSION;
  static constexpr const char* channel = APP_CHANNEL;
  static constexpr const char* board = APP_BOARD;
  static constexpr const char* hardwareType = APP_HARDWARE_TYPE;
};
```

Update `platformio.ini` so envs define `APP_HARDWARE_TYPE`, and add the new pre-script before resource embedding:

```ini
extra_scripts =
  pre:scripts/generate_version.py
  pre:scripts/embed_resources.py
```

Create `scripts/generate_version.py` in the same style as `microbbox`, but emitting `APP_VERSION`, `APP_CHANNEL`, `APP_BOARD`, and `APP_HARDWARE_TYPE` safely for one current hardware target and future env overrides.

**Step 4: Keep resource placeholders aligned with generated version**

Ensure `scripts/embed_resources.py` still reads version/channel from the active build defines so embedded frontend assets show the correct generated values.

**Step 5: Re-run the focused test and verify it passes**

Run: `cd /home/builder/mylamp && /home/builder/.platformio/penv/bin/pio test -e native-test -f test_status_json`

Expected: PASS.

**Step 6: Commit**

```bash
cd /home/builder/mylamp
git add scripts/generate_version.py platformio.ini include/BuildInfo.h scripts/embed_resources.py test/test_status_json/test_main.cpp
git commit -m "Add OTA build identity generation"
```

### Task 2: Add update settings and persistence

**Files:**
- Modify: `include/settings/AppSettings.h`
- Modify: `include/settings/AppSettingsPersistence.h`
- Modify: `src/settings/AppSettingsPersistence.cpp`
- Modify: `src/settings/PreferencesSettingsBackend.cpp`
- Test: `test/test_settings_persistence/test_main.cpp`

**Step 1: Write the failing settings persistence test**

Add a test ensuring update channel persists and defaults sanely.

```cpp
TEST_ASSERT_EQUAL_STRING("stable", loaded.update.channel.c_str());
```

**Step 2: Run the focused test and verify it fails**

Run: `cd /home/builder/mylamp && /home/builder/.platformio/penv/bin/pio test -e native-test -f test_settings_persistence`

Expected: FAIL because `AppSettings` currently has no OTA/update section.

**Step 3: Add OTA-related settings**

Extend `AppSettings` with a nested update section.

```cpp
struct UpdateSettings {
  std::string channel = "stable";
  bool autoCheckEnabled = true;
};

struct AppSettings {
  network::NetworkSettings network;
  time::ClockSettings clock;
  UpdateSettings update;
};
```

Update persistence to read/write the new keys, keeping backward compatibility if preferences do not exist yet.

**Step 4: Re-run the focused test and verify it passes**

Run: `cd /home/builder/mylamp && /home/builder/.platformio/penv/bin/pio test -e native-test -f test_settings_persistence`

Expected: PASS.

**Step 5: Commit**

```bash
cd /home/builder/mylamp
git add include/settings/AppSettings.h include/settings/AppSettingsPersistence.h src/settings/AppSettingsPersistence.cpp src/settings/PreferencesSettingsBackend.cpp test/test_settings_persistence/test_main.cpp
git commit -m "Persist OTA channel settings"
```

### Task 3: Add release metadata model and parser

**Files:**
- Create: `include/update/FirmwareReleaseInfo.h`
- Create: `include/update/GitHubReleaseParser.h`
- Create: `src/update/GitHubReleaseParser.cpp`
- Modify: `platformio.ini`
- Test: `test/test_github_release_parser/test_main.cpp`

**Step 1: Write the failing parser tests**

Create parser tests using fixed JSON payloads modeled after GitHub Releases responses.

```cpp
void test_parser_selects_matching_stable_asset_for_hardware();
void test_parser_selects_matching_dev_asset_for_hardware();
void test_parser_reports_missing_hardware_asset();
void test_parser_ignores_older_release_than_current_version();
```

**Step 2: Run the new test target and verify it fails**

Run: `cd /home/builder/mylamp && /home/builder/.platformio/penv/bin/pio test -e native-test -f test_github_release_parser`

Expected: FAIL because parser classes do not exist yet.

**Step 3: Implement the domain model and parser**

Create `FirmwareReleaseInfo` with fields such as:

```cpp
struct FirmwareReleaseInfo {
  bool available = false;
  std::string channel;
  std::string version;
  std::string assetName;
  std::string assetUrl;
  std::string checksumUrl;
  std::string notes;
  std::string error;
};
```

Implement `GitHubReleaseParser` to:

- accept current version, channel, hardware type;
- scan GitHub release JSON;
- select the matching asset name pattern;
- distinguish `no update`, `missing asset`, and `invalid payload`.

Update `platformio.ini` native test `build_src_filter` to include `src/update/*.cpp`.

**Step 4: Re-run the parser tests and verify they pass**

Run: `cd /home/builder/mylamp && /home/builder/.platformio/penv/bin/pio test -e native-test -f test_github_release_parser`

Expected: PASS.

**Step 5: Commit**

```bash
cd /home/builder/mylamp
git add include/update/FirmwareReleaseInfo.h include/update/GitHubReleaseParser.h src/update/GitHubReleaseParser.cpp platformio.ini test/test_github_release_parser/test_main.cpp
git commit -m "Add GitHub release metadata parser"
```

### Task 4: Add firmware update service contracts

**Files:**
- Create: `include/update/IFirmwareReleaseSource.h`
- Create: `include/update/IFirmwareInstaller.h`
- Create: `include/update/FirmwareUpdateService.h`
- Create: `src/update/FirmwareUpdateService.cpp`
- Test: `test/test_firmware_update_service/test_main.cpp`

**Step 1: Write the failing service tests**

Create tests for service behavior with fake source/installer implementations.

```cpp
void test_check_returns_available_release_when_source_finds_newer_build();
void test_install_requires_available_release();
void test_install_propagates_installer_error();
```

**Step 2: Run the new tests and verify they fail**

Run: `cd /home/builder/mylamp && /home/builder/.platformio/penv/bin/pio test -e native-test -f test_firmware_update_service`

Expected: FAIL because the update service contracts do not exist.

**Step 3: Implement the minimal service layer**

Create interfaces:

```cpp
class IFirmwareReleaseSource {
 public:
  virtual ~IFirmwareReleaseSource() = default;
  virtual FirmwareReleaseInfo check(const BuildIdentity&, const settings::UpdateSettings&) = 0;
};

class IFirmwareInstaller {
 public:
  virtual ~IFirmwareInstaller() = default;
  virtual bool install(const FirmwareReleaseInfo&, std::string& error) = 0;
};
```

Implement `FirmwareUpdateService` to own current state, last check result, and install result.

**Step 4: Re-run the service tests and verify they pass**

Run: `cd /home/builder/mylamp && /home/builder/.platformio/penv/bin/pio test -e native-test -f test_firmware_update_service`

Expected: PASS.

**Step 5: Commit**

```bash
cd /home/builder/mylamp
git add include/update/IFirmwareReleaseSource.h include/update/IFirmwareInstaller.h include/update/FirmwareUpdateService.h src/update/FirmwareUpdateService.cpp test/test_firmware_update_service/test_main.cpp
git commit -m "Add firmware update service layer"
```

### Task 5: Add ESP32 GitHub release source and OTA installer

**Files:**
- Create: `include/update/ArduinoGitHubReleaseSource.h`
- Create: `src/update/ArduinoGitHubReleaseSource.cpp`
- Create: `include/update/Esp32FirmwareInstaller.h`
- Create: `src/update/Esp32FirmwareInstaller.cpp`
- Modify: `platformio.ini`
- Test: `test/test_update_status_json/test_main.cpp`

**Step 1: Write a failing serialization/status test for runtime update state**

Add a test proving update state can surface to JSON once runtime wiring exists.

```cpp
TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(json.find("\"updateState\":\"idle\"")));
TEST_ASSERT_NOT_EQUAL(-1, static_cast<int>(json.find("\"hardwareType\":\"c3-cylinder32x16\"")));
```

**Step 2: Run the test and verify it fails**

Run: `cd /home/builder/mylamp && /home/builder/.platformio/penv/bin/pio test -e native-test -f test_update_status_json`

Expected: FAIL because the update status fields are not in the status snapshot yet.

**Step 3: Implement the Arduino integrations**

Create `ArduinoGitHubReleaseSource` around `WiFiClientSecure` and `HTTPClient` to fetch GitHub Releases JSON.

Create `Esp32FirmwareInstaller` around `Update.h`/ESP OTA flow to:

- open HTTPS stream;
- write OTA image;
- validate result;
- return explicit error text instead of silent failure.

Do not wire auto-install. Keep install manual only.

**Step 4: Re-run the focused test and verify it still fails for the expected reason**

Run: `cd /home/builder/mylamp && /home/builder/.platformio/penv/bin/pio test -e native-test -f test_update_status_json`

Expected: still FAIL, but only because main/status wiring is not done yet.

**Step 5: Commit the runtime integration layer**

```bash
cd /home/builder/mylamp
git add include/update/ArduinoGitHubReleaseSource.h src/update/ArduinoGitHubReleaseSource.cpp include/update/Esp32FirmwareInstaller.h src/update/Esp32FirmwareInstaller.cpp platformio.ini test/test_update_status_json/test_main.cpp
git commit -m "Add Arduino GitHub OTA integration layer"
```

### Task 6: Wire OTA state into main runtime and web API

**Files:**
- Modify: `src/main.cpp`
- Modify: `include/web/StatusJsonBuilder.h`
- Modify: `src/web/StatusJsonBuilder.cpp`
- Modify: `include/web/LampWebServer.h`
- Modify: `src/web/LampWebServer.cpp`
- Test: `test/test_status_json/test_main.cpp`

**Step 1: Extend the failing status test to cover update fields**

Add assertions for:

```cpp
"hardwareType"
"updateState"
"updateChannel"
"availableVersion"
"updateError"
```

**Step 2: Run the focused test and verify it fails**

Run: `cd /home/builder/mylamp && /home/builder/.platformio/penv/bin/pio test -e native-test -f test_status_json`

Expected: FAIL.

**Step 3: Add OTA state to status snapshot and HTTP routes**

Extend `StatusSnapshot` with update fields.

Register new routes in `LampWebServer`:

```cpp
server_.on("/api/update/status", HTTP_GET, [this]() { handleUpdateStatus(); });
server_.on("/api/update/check", HTTP_POST, [this]() { handleUpdateCheck(); });
server_.on("/api/update/install", HTTP_POST, [this]() { handleUpdateInstall(); });
```

Wire `src/main.cpp` so it creates the update service, injects it into the web layer, and refreshes status snapshot after checks/install attempts.

**Step 4: Re-run the focused test and verify it passes**

Run: `cd /home/builder/mylamp && /home/builder/.platformio/penv/bin/pio test -e native-test -f test_status_json`

Expected: PASS.

**Step 5: Commit**

```bash
cd /home/builder/mylamp
git add src/main.cpp include/web/StatusJsonBuilder.h src/web/StatusJsonBuilder.cpp include/web/LampWebServer.h src/web/LampWebServer.cpp test/test_status_json/test_main.cpp
git commit -m "Expose OTA state through firmware web API"
```

### Task 7: Add frontend OTA controls and mock API support

**Files:**
- Modify: `frontend/src/dev/mockTypes.ts`
- Modify: `frontend/src/dev/mockScenarios.ts`
- Modify: `frontend/src/main.ts`
- Modify: `frontend/mockApi.mjs`
- Modify: `frontend/src/styles/app.css`

**Step 1: Add a failing frontend build expectation**

Introduce the new typed fields in `frontend/src/dev/mockTypes.ts` first so the build fails until UI and mock data are updated.

**Step 2: Run frontend build and verify it fails**

Run: `cd /home/builder/mylamp/frontend && npm run build`

Expected: FAIL with missing OTA fields/usages.

**Step 3: Implement minimal OTA UI**

Update the UI to show:

- current version, channel, hardware type;
- current OTA state;
- check updates button;
- install button if an update is available;
- dev channel warning text.

Update mock API endpoints for `/api/update/status`, `/api/update/check`, and `/api/update/install` so local frontend flow remains usable without hardware.

**Step 4: Re-run frontend build and verify it passes**

Run: `cd /home/builder/mylamp/frontend && npm run build`

Expected: PASS.

**Step 5: Commit**

```bash
cd /home/builder/mylamp
git add frontend/src/dev/mockTypes.ts frontend/src/dev/mockScenarios.ts frontend/src/main.ts frontend/mockApi.mjs frontend/src/styles/app.css
git commit -m "Add OTA controls to embedded web UI"
```

### Task 8: Add release helper scripts and GitHub Actions workflows

**Files:**
- Create: `.github/workflows/ci.yml`
- Create: `.github/workflows/dev-build.yml`
- Create: `.github/workflows/release.yml`
- Create: `.github/workflows/branch-protection.yml`
- Create: `.github/workflows/create-support-branch.yml`
- Create: `.github/workflows/backport-hotfix.yml`
- Create: `.github/workflows/version-support-check.yml`
- Create: `scripts/calculate_next_version.sh`
- Modify: `README.md`
- Modify: `docs/ARCHITECTURE.md`

**Step 1: Write the workflow contract in the files first**

Create the workflow files with the exact triggers and artifact naming rules from the design, initially with a single active hardware target in the matrix.

**Step 2: Run YAML validation and shell syntax checks**

Run:

```bash
cd /home/builder/mylamp
python - <<'PY'
import pathlib, yaml
for path in pathlib.Path('.github/workflows').glob('*.yml'):
    yaml.safe_load(path.read_text())
print('workflow yaml ok')
PY
bash -n scripts/calculate_next_version.sh
```

Expected: success.

**Step 3: Ensure CI commands match the real repo**

`ci.yml` should run the actual commands that already pass locally:

```bash
/home/runner/.platformio/penv/bin/pio test -e native-test
cd frontend && npm ci && npm run build
/home/runner/.platformio/penv/bin/pio run -e esp32-c3-supermini-dev
```

If needed, install PlatformIO in the workflow via pip rather than relying on preinstalled binaries.

**Step 4: Update docs to explain OTA/release flow**

Update `README.md` and `docs/ARCHITECTURE.md` with:

- channels;
- hardware-specific artifact naming;
- release source = GitHub Releases;
- branch model.

**Step 5: Commit**

```bash
cd /home/builder/mylamp
git add .github/workflows scripts/calculate_next_version.sh README.md docs/ARCHITECTURE.md
git commit -m "Add OTA GitHub Actions release pipeline"
```

### Task 9: Full verification and device validation

**Files:**
- Modify as needed: only if verification uncovers defects

**Step 1: Run the native OTA-related tests**

Run:

```bash
cd /home/builder/mylamp
/home/builder/.platformio/penv/bin/pio test -e native-test -f test_status_json
/home/builder/.platformio/penv/bin/pio test -e native-test -f test_settings_persistence
/home/builder/.platformio/penv/bin/pio test -e native-test -f test_github_release_parser
/home/builder/.platformio/penv/bin/pio test -e native-test -f test_firmware_update_service
```

Expected: all PASS.

**Step 2: Run the full verification set**

Run:

```bash
cd /home/builder/mylamp/frontend && npm run build
cd /home/builder/mylamp && /home/builder/.platformio/penv/bin/pio run -e esp32-c3-supermini-dev
```

Expected: PASS.

**Step 3: Flash and validate on hardware**

Run:

```bash
cd /home/builder/mylamp
/home/builder/.platformio/penv/bin/pio run -e esp32-c3-supermini-dev -t upload
```

Manual checks:

- `/api/status` includes OTA fields;
- `/api/update/check` returns structured result;
- UI shows hardware type and channel;
- stable/dev channel selection persists;
- device does not crash when GitHub is unreachable.

**Step 4: Request code review before merge**

Use the `requesting-code-review` skill before finishing the branch.

**Step 5: Commit any final fixes**

```bash
cd /home/builder/mylamp
git add .
git commit -m "Polish OTA release flow verification fixes"
```