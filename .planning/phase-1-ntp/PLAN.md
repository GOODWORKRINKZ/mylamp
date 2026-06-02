# PLAN: Phase 1 — NTP Sync

**Phase:** 1
**Created:** 2026-06-02
**Status:** Ready
**Requirements:** NTP-01, NTP-02
**Depends on:** None

---

## 1. Research Summary

### 1.1 Current State (Live Device Audit)

| Component | Status | Notes |
|-----------|--------|-------|
| `TimePlanner` | ✅ Works | Correctly sets `ntpSyncEnabled=true` when Wi-Fi client mode active |
| `TimeRuntimeService` | ✅ Works | Calls `syncTime()` every 30s, reports results correctly |
| `StatusJsonBuilder` | ✅ Works | Reports `currentTime` and `clockStatus` in `/api/status` |
| `/api/settings/time` GET/POST | ✅ Works | Timezone get/set works, valid values: UTC0, EET-2…, MSK-3, CET-1…, EST5…, PST8… |
| `ClockOverlay` | ✅ Works | Renders HH:MM digits on LED matrix when time valid |
| **`ArduinoNtpTimeSource::syncTime()`** | ❌ **BROKEN** | **Root cause** — NTP never completes sync |
| `/api/time` endpoint | ❌ Missing | Returns 404, no dedicated time endpoint |
| `currentTime` in `/api/status` | ❌ Empty | Always `""` — time never becomes valid |

### 1.2 Root Cause Analysis

**Git history**: Commit `bd79f35` changed `ArduinoNtpTimeSource::syncTime()` from:

```cpp
// OLD (working):
tm timeinfo{};
if (!getLocalTime(&timeinfo, 1000)) {  // blocks up to 1s, polls every 10ms
    return validTime_;
}
```

To:

```cpp
// NEW (broken):
time_t now_epoch;
::time(&now_epoch);
if (now_epoch < 1700000000) {  // Nov 2023 threshold — non-blocking
    return validTime_;
}
```

**Why this breaks NTP:**
1. `configTzTime("MSK-3", "pool.ntp.org", "time.nist.gov")` starts the **asynchronous** SNTP client
2. `::time(&now_epoch)` is called **immediately** — SNTP hasn't synced yet (takes 2-10 seconds)
3. Epoch < 1,700,000,000 → returns `false`, `validTime_` stays `false` forever
4. On subsequent calls (30s later), `configTzTime` is **skipped** (same timezone), but the SNTP client should have synced by then
5. **However**: `sntp_get_sync_status()` is never checked — the code relies solely on the epoch heuristic which is unreliable

**Secondary issues:**
- No `sntp_get_sync_status()` check — can't distinguish "SNTP not started" from "SNTP failed"
- No sync interval configuration — LWIP default may be very long (hours)
- DNS resolution of `pool.ntp.org` may fail silently on ESP32-C3
- No retry/backoff mechanism
- `validTime_` flag is never reset when sync is lost

### 1.3 ESP32 SNTP API Available

The ESP-IDF/LWIP SNTP API provides (from `esp_sntp.h`):

| Function | Purpose |
|----------|---------|
| `sntp_get_sync_status()` | Returns `SNTP_SYNC_STATUS_RESET` / `COMPLETED` / `IN_PROGRESS` |
| `sntp_set_sync_interval(uint32_t ms)` | Sets poll interval (min 15s per RFC 4330) |
| `sntp_set_time_sync_notification_cb(cb)` | Callback when sync completes |
| `sntp_restart()` | Restart SNTP without full re-init |

---

## 2. Tasks

### Task 1: Fix `ArduinoNtpTimeSource` core sync logic

**File:** `src/time/ArduinoNtpTimeSource.cpp`

**Changes:**
1. Add `#include "lwip/apps/sntp.h"` for `sntp_get_sync_status()`
2. Replace epoch heuristic with `sntp_get_sync_status()` check
3. On first call (or timezone change): call `configTzTime()`, then poll `sntp_get_sync_status()` with a 5-second timeout
4. On subsequent calls: check `sntp_get_sync_status()`, if `COMPLETED` → cache formatted time, set `validTime_ = true`
5. If status is `RESET` (sync lost): trigger `sntp_restart()` or re-call `configTzTime()`
6. Set `sntp_set_sync_interval(3600000)` — 1 hour poll interval (reasonable for clock drift)

**New flow:**
```
syncTime(timezone, server1, server2):
  if timezone changed or not configured:
    configTzTime(timezone, server1, server2)
    sntp_set_sync_interval(3600000)
    wait up to 5000ms for sntp_get_sync_status() == COMPLETED
  
  if sntp_get_sync_status() == COMPLETED:
    format time → validTime_ = true
    return true
  else:
    return validTime_  // keep previous state
```

### Task 2: Add `/api/time` endpoint

**Files:** `src/web/LampWebServer.cpp`, new file `src/web/TimeStatusJson.cpp`

**Changes:**
1. Add `GET /api/time` route handler
2. Return JSON with:
   - `currentTime` (formatted HH:MM:SS)
   - `timezone` (current timezone string)
   - `syncStatus` (ntp_synced / ntp_pending / ntp_failed / ntp_disabled / cached)
   - `ntpServer` (primary server hostname)
   - `epoch` (unix timestamp)
3. Wire into `LampWebServer::registerRoutes()`

### Task 3: Update `StatusSnapshot` with sync status

**Files:** `include/web/StatusSnapshot.h`, `src/web/StatusJsonBuilder.cpp`

**Changes:**
1. Add `syncStatus` field to `StatusSnapshot` (replaces generic `clockStatus` string for NTP state)
2. `StatusJsonBuilder` returns `syncStatus` in `/api/status`
3. Keep `clockStatus` for backward compatibility

### Task 4: Add sync status to web UI

**Files:** `frontend/src/main.ts`

**Changes:**
1. Read `syncStatus` from `/api/status`
2. Display sync status in the Clock dialog (e.g., "NTP: синхронизировано" / "NTP: ожидание…")
3. Show current time in the status bar when available

### Task 5: Add test for `ArduinoNtpTimeSource`

**File:** `test/test_arduino_ntp/test_main.cpp` (new)

**Changes:**
1. Unit tests for `syncTime()` with mocked `configTzTime` / `sntp_get_sync_status`
2. Test cases:
   - First call starts SNTP, waits for sync, formats time
   - Second call uses cached time without re-initializing
   - Timezone change triggers re-init
   - Sync timeout → returns previous validTime_
   - SNTP reset after completed sync → restart

### Task 6: Update docs

**File:** `docs/STATUS.md`

**Changes:** Mark NTP-01 and NTP-02 as validated.

---

## 3. Verification Plan

### Unit Tests
- `test_arduino_ntp` — `ArduinoNtpTimeSource::syncTime()` all scenarios
- `test_time_runtime` — existing tests must still pass
- `test_time_policy` — existing tests must still pass
- `test_time_settings_api` — existing tests must still pass

### Integration Tests (on device)
1. **NTP-01**: Flash firmware → connect to Wi-Fi → wait ≤30s → `/api/time` returns `syncStatus: "ntp_synced"` and valid `currentTime`
2. **NTP-01**: Reboot device → time should re-sync within 30s
3. **NTP-02**: Clock overlay renders HH:MM on LED matrix when time is valid
4. **NTP-02**: `/api/time` returns correct time matching real world (within 1-2 seconds)
5. **Edge case**: Change timezone via web UI → time re-syncs and shows correct local time
6. **Edge case**: Wi-Fi disconnected → `syncStatus` reports appropriate state, cached time used if available

### Nyquist Validation
- Each requirement (NTP-01, NTP-02) must have at least one test
- NTP-01: sync within 30s → verified by integration test
- NTP-02: time provided to other services → verified by unit + integration test

---

## 4. Risk Analysis

| Risk | Severity | Mitigation |
|------|----------|------------|
| DNS resolution fails on ESP32 | Medium | Use IP fallback servers, add DNS check |
| SNTP never syncs due to firewall | Medium | Add diagnostic endpoint, document troubleshooting |
| 5-second initial wait blocks main loop | Low | Only on first call; subsequent calls are non-blocking; ESP32 main loop is not time-critical at startup |
| Memory overhead of new endpoint | Low | Adding one small JSON endpoint, negligible |
| Regression in existing time tests | Low | All existing tests must pass before merging |

---

## 5. Task Execution Order

```
Task 1 (core fix) ──→ Task 2 (endpoint) ──→ Task 3 (snapshot)
                          │
                          └──→ Task 4 (UI update)
                          
Task 5 (tests) ── can be done in parallel with Tasks 2-4

Task 6 (docs) ── last, after all verified
```

**Parallel opportunities:** Tasks 2+3+4 can be done together after Task 1. Task 5 is independent.

---

## 6. Success Criteria (from ROADMAP.md)

- [ ] Устройство синхронизирует время с NTP-сервером в течение 30 секунд после подключения к Wi-Fi
- [ ] `TimeRuntimeService::getCurrentTime()` возвращает корректное время
- [ ] Время не сбрасывается до перезагрузки устройства

---

*Plan created: 2026-06-02 based on live device audit and codebase analysis*
