# Technology Stack

**Analysis Date:** 2026-06-02

## Languages

**Primary:**
- C++ (C++11/C++17) - Embedded firmware on ESP32-C3. All core logic: LED control, networking, web server, DSL runtime, sensors, OTA updates. See `src/` and `include/`.
- TypeScript 4.9.5 - Frontend SPA (single-page application). Type-checked, compiled by Vite. See `frontend/src/`.

**Secondary:**
- Python 3 - Build-time scripts: version generation (`scripts/generate_version.py`) and resource embedding (`scripts/embed_resources.py`).
- JavaScript (ESM) - Frontend test scripts: `frontend/test/layout.test.mjs`, `frontend/test/mock-api.test.mjs`.
- HTML/CSS - Frontend markup and styles. `frontend/index.html`, `frontend/src/styles/app.css`.

## Runtime

**Environment (Embedded):**
- Espressif32 platform (ESP32-C3 Super Mini)
- Arduino framework
- Build system: PlatformIO (`platformio.ini`)
- Monitor baud rate: 115200

**Environment (Frontend Dev):**
- Node.js 20 (CI/CD), any Node.js for local dev
- Vite 2.9.18 dev server with HMR

**Package Manager:**
- npm (Node.js) - `frontend/package.json`
- Lockfile: `frontend/package-lock.json` (present)
- PlatformIO Library Manager - `platformio.ini` `lib_deps`

## Frameworks

**Core (Embedded):**
- Arduino Framework (Espressif32) - Hardware abstraction, WiFi, HTTP client, filesystem, OTA
- FastLED ^3.7.0 - WS2812B LED strip control, pixel rendering pipeline
- ArduinoJson ^6.21.5 - JSON parsing and serialization for API requests/responses, GitHub API parsing, settings persistence
- WebServer (ESP32 built-in) - HTTP server serving the SPA and REST API endpoints. See `include/web/LampWebServer.h`, `src/web/LampWebServer.cpp`

**Core (Frontend):**
- Vite 2.9.18 - Build tool and dev server. No additional UI framework (vanilla TypeScript DOM manipulation). See `frontend/vite.config.ts`
- Custom Vite mock API plugin (`frontend/mockApi.mjs`) - Simulates device backend for offline frontend development

**Testing (Embedded):**
- Unity Test Framework (PlatformIO native) - Unit tests for C++ modules. See `test/` directories and `platformio.ini` `[env:native-test]`

**Testing (Frontend):**
- Custom Node.js test scripts (.mjs) - Layout tests and mock API tests. See `frontend/test/`. No test framework (Jest/Mocha/Vitest) detected.

**Build/Dev:**
- PlatformIO CLI (`pio`) - Compile, upload, test
- npm scripts - `npm run dev` (Vite dev server), `npm run build` (production build)
- Python `embed_resources.py` - Gzip-compresses and embeds frontend build output (`resources/dist/`) into C header (`include/embedded_resources.h`) as PROGMEM byte arrays
- Python `generate_version.py` - Generates version info from git into `include/BuildInfo.h`

## Key Dependencies

**Critical (Embedded):**
- `FastLED` ^3.7.0 - Core LED rendering. Drives WS2812B matrix at `include/AppConfig.h` configured pin. Used in `src/main.cpp` for `CRGB g_leds[]` array.
- `ArduinoJson` ^6.21.5 - All JSON processing: GitHub release API, live request/response, preset/playlist serialization, network/time settings. Used across `src/web/`, `src/update/`, `src/live/`.
- `Adafruit AHTX0` ^2.0.6 - AHT30 temperature/humidity sensor driver over I2C. See `src/sensors/ArduinoAht30SensorSource.cpp`.
- `LittleFS` - Flash filesystem for preset/playlist persistence. See `src/storage/LittleFsFileStore.cpp`.
- `Preferences` - NVS key-value storage for settings persistence. See `src/settings/PreferencesSettingsBackend.cpp`.
- `WiFi` / `WiFiClientSecure` / `HTTPClient` / `WebServer` - ESP32 Arduino network stack. TLS via x509 certificate bundle.
- `Update` (ESP32 Core) - OTA firmware flashing. See `src/update/Esp32FirmwareInstaller.cpp`.
- `mbedtls/sha256` - SHA-256 checksum verification for OTA firmware. See `src/update/Esp32FirmwareInstaller.cpp`.

**Infrastructure:**
- PlatformIO - Cross-platform embedded build system
- Git - Version control, version stamping (`scripts/generate_version.py` reads git tags/commits)
- GitHub Actions - CI/CD pipelines (`.github/workflows/`)

**Frontend (dev only):**
- `typescript` 4.9.5 - Type checking. `frontend/tsconfig.json` with `strict: true`, target ES2020, noEmit.
- `vite` 2.9.18 - Build and dev server.

## Configuration

**Environment (Embedded Build):**
- Build flags in `platformio.ini` define:
  - `APP_CHANNEL` - `"dev"` or `"stable"` (per environment)
  - `APP_BOARD` - `"esp32-c3-supermini"`
  - `APP_HARDWARE_TYPE` - `"c3-cylinder32x16"`
  - `APP_GITHUB_REPO` - `"GOODWORKRINKZ/mylamp"`
- Hardware pin configuration in `include/AppConfig.h`:
  - LED data pin: GPIO 2
  - I2C SDA: GPIO 8, SCL: GPIO 9
  - Matrix: 2 panels of 16×16, logical 32×16 pixels
- Partition table: `partitions_ota.csv` (OTA with two app slots + LittleFS)

**Environment (Frontend):**
- `frontend/tsconfig.json` - TypeScript compiler options (strict, ES2020 target, ESNext modules)
- `frontend/vite.config.ts` - Build outputs to `../resources/dist/`, single JS bundle as `script.js`, single CSS as `styles.css`, relative base path `./`

**Build:**
- `platformio.ini` - Three environments: `esp32-c3-supermini-dev`, `esp32-c3-supermini-release`, `native-test`
- Pre-build scripts: `generate_version.py` (runs first), `embed_resources.py` (runs second)
- Frontend must be built (`npm run build` in `frontend/`) before firmware compilation

**Key Config Files:**
- `platformio.ini` - Build environments, library dependencies, build flags, test source filter
- `partitions_ota.csv` - ESP32 flash partition layout
- `frontend/tsconfig.json` - TypeScript configuration
- `frontend/vite.config.ts` - Vite build configuration with mock API plugin
- `include/AppConfig.h` - Hardware constants (pins, panel dimensions, timing)

## Platform Requirements

**Development:**
- PlatformIO CLI (`pip install platformio`)
- Node.js 20+ with npm
- Python 3.11+ (for build scripts)
- USB connection to ESP32-C3 Super Mini for flashing

**Production (Device):**
- ESP32-C3 Super Mini (RISC-V)
- 2× WS2812B 16×16 LED matrices
- AHT30 temperature/humidity sensor (I2C)
- Wi-Fi 2.4 GHz
- Flash: 4 MB with OTA partition scheme

**Production (CI/CD):**
- GitHub Actions Ubuntu runner
- `softprops/action-gh-release@v2` for release publishing
- `actions/upload-artifact@v4` for dev build artifacts

---

*Stack analysis: 2026-06-02*
