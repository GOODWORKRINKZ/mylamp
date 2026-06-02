# External Integrations

**Analysis Date:** 2026-06-02

## APIs & External Services

**GitHub Releases API (OTA Firmware Updates):**
- Service: GitHub REST API v3
  - Endpoint: `https://api.github.com/repos/{owner}/{repo}/releases`
  - For stable channel: `GET /repos/{repo}/releases/latest`
  - For dev channel: `GET /repos/{repo}/releases?per_page=3`
  - SDK/Client: `HTTPClient` + `WiFiClientSecure` (ESP32 Arduino), custom parser (`update/GitHubReleaseParser`)
  - Auth: None (public repository)
  - Implementation: `src/update/ArduinoGitHubReleaseSource.cpp`
  - Parser: `src/update/GitHubReleaseParser.cpp` - Parses JSON response via ArduinoJson, filters by channel (stable/prerelease), matches hardware type in asset names
  - Asset naming convention: `mylamp-{hardwareType}-{version}-release.bin` (stable) or `mylamp-{hardwareType}-dev-{version}.bin` (dev)
  - Checksum: SHA-256 via `{assetName}.sha256` companion file

**NTP Time Servers:**
- Primary: `pool.ntp.org`
- Secondary: `time.nist.gov`
- SDK/Client: ESP32 built-in `configTzTime()` (Arduino core)
- Implementation: `src/time/ArduinoNtpTimeSource.cpp`
- Refresh interval: 30 seconds (`kTimeRefreshIntervalMs` in `include/AppConfig.h`)
- Timezone: Configurable, default `UTC0`

## Data Storage

**LittleFS Flash Filesystem:**
- Type: Embedded flash filesystem (ESP32 LittleFS)
- Client: `fs::FS` (ESP32 Arduino), wrapped in `LittleFsFileStore` (`src/storage/LittleFsFileStore.cpp`)
- Partition: `littlefs` at offset 0x310000, size 0x0F0000 (960 KB)
- Used for: Preset effect files, playlist files, content storage
- File operations: writeText, readText, remove, list

**Preferences/NVS (Non-Volatile Storage):**
- Type: ESP32 NVS key-value store
- Client: `Preferences` (ESP32 Arduino), wrapped in `PreferencesSettingsBackend` (`src/settings/PreferencesSettingsBackend.cpp`)
- Namespace: `"mylamp"`
- Used for: Wi-Fi settings, timezone, update channel preference, persistent app settings

**File Storage:**
- Local filesystem only (LittleFS on embedded flash)
- No external cloud storage or file service

**Caching:**
- None (no in-memory cache beyond program state)

## Authentication & Identity

**Auth Provider:**
- Custom (no external auth provider)
- Device operates as open Wi-Fi access point with password (`MYLAMP` SSID prefix, password `12345678` from `include/AppConfig.h`)
- When in station mode, connects to user-provided Wi-Fi credentials
- No user authentication on web UI or API endpoints
- GitHub API access is unauthenticated (public repository reads only)

## Monitoring & Observability

**Error Tracking:**
- None (no external error tracking service)
- Serial console output for debug messages (`Serial.println()`)
- Status JSON endpoint (`/api/status`) reports device state including update errors, sensor status, live error summary

**Logs:**
- Serial monitor at 115200 baud
- No persistent logging to filesystem
- No structured logging framework

## CI/CD & Deployment

**Hosting:**
- GitHub Releases - Firmware binary distribution
  - Stable releases: Tag-triggered (`v*.*.*`), publishes `.bin` + `.sha256` as release assets
  - Dev builds: Push-triggered (`develop`, `feature/**`), publishes as prerelease with `actions/upload-artifact`
- Frontend is embedded in firmware (no separate hosting)

**CI Pipeline:**
- Service: GitHub Actions (`.github/workflows/`)
- Workflows:
  - `ci.yml` - Runs on push to `main`/`develop`/`feature/**` and PRs. Two jobs: native tests (`pio test -e native-test`) and firmware build verification (builds both dev and release environments)
  - `release.yml` - Triggered on `v*.*.*` tag push. Builds release firmware, creates GitHub Release with auto-generated notes via `softprops/action-gh-release@v2`
  - `dev-build.yml` - Triggered on push to `develop`/`feature/**` or manual `workflow_dispatch`. Builds dev firmware, uploads artifacts, publishes prerelease

**CI Dependencies:**
- `actions/checkout@v4` with `fetch-depth: 0`
- `actions/setup-python@v5` (Python 3.11)
- `actions/setup-node@v4` (Node 20, npm cache on `frontend/package-lock.json`)
- `softprops/action-gh-release@v2`
- `actions/upload-artifact@v4`

## Environment Configuration

**Required build-time defines (in `platformio.ini`):**
- `APP_CHANNEL` - `"dev"` or `"stable"`
- `APP_BOARD` - `"esp32-c3-supermini"`
- `APP_HARDWARE_TYPE` - `"c3-cylinder32x16"`
- `APP_GITHUB_REPO` - `"GOODWORKRINKZ/mylamp"`

**Runtime configuration (stored in NVS):**
- Wi-Fi: SSID, password, mode (AP/Station)
- Timezone string
- Update channel preference (`"stable"` or `"dev"`)

**Runtime configuration (stored in LittleFS):**
- Preset effect definitions
- Playlist definitions

**Secrets location:**
- Wi-Fi access point password is hardcoded in `include/AppConfig.h` (`kAccessPointPassword = "12345678"`)
- No external secrets management detected
- GitHub Actions uses default `GITHUB_TOKEN` for release publishing (no custom secrets)

## Webhooks & Callbacks

**Incoming:**
- None. The device runs an HTTP server (`WebServer` on port 80) serving:
  - Static SPA files (`/`, `/favicon.svg`, `/script.js`, `/styles.css`)
  - REST API endpoints:
    - `GET /api/status` - Device status (version, network, sensors, active effect)
    - `GET/POST /api/network` - Wi-Fi settings
    - `GET/POST /api/time` - Timezone settings
    - `GET/POST /api/update/settings` - Update channel settings
    - `GET /api/update/current` - Current firmware info
    - `POST /api/update/check` - Check for firmware updates
    - `POST /api/update/install` - Install firmware update
    - `POST /api/live/validate` - Validate DSL effect source
    - `POST /api/live/run` - Run DSL effect temporarily
    - `GET/PUT /api/presets` - List/create presets
    - `GET/PUT/DELETE /api/presets/{id}` - Preset CRUD
    - `GET/PUT/DELETE /api/playlists/{id}` - Playlist CRUD
  - All API responses are JSON (`application/json`)

**Outgoing:**
- GitHub API (`api.github.com`) - HTTPS GET for release info, initiated by user action or on-demand
- NTP servers (`pool.ntp.org`, `time.nist.gov`) - Periodic time sync (every 30s when Wi-Fi connected)
- HTTPS download of firmware `.bin` assets from `github.com` for OTA updates

## Sensors (On-Device)

**AHT30 Temperature/Humidity Sensor:**
- Protocol: I2C (SDA: GPIO 8, SCL: GPIO 9)
- Library: `Adafruit_AHTX0` ^2.0.6
- Implementation: `src/sensors/ArduinoAht30SensorSource.cpp`
- Refresh interval: 5 seconds (`kSensorRefreshIntervalMs` in `include/AppConfig.h`)
- Stale read limit: 12 attempts before marking unavailable
- Valid range: -40°C to +125°C, 0% to 100% RH

---

*Integration audit: 2026-06-02*
