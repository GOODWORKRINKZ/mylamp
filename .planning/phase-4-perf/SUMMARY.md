# SUMMARY: Phase 4 — Perf & Bugs

**Phase:** 4
**Completed:** 2026-06-02
**Status:** Complete
**Requirements:** PERF-01, PERF-02, PERF-03

---

## What Was Done

### PERF-01: Clear framebuffer on effect transition
- **EffectRegistry** (`include/effects/EffectRegistry.h`, `src/effects/EffectRegistry.cpp`):
  - Added `clearRequested_` flag and `requestClear()` method
  - `setActiveByName()` sets `clearRequested_ = true` when switching to a different effect
  - `renderActive()` checks flag, clears FB if set, then renders
- **renderEffectPass** (`src/main.cpp`):
  - Added `g_wasLiveActive` tracking to detect live→compiled transitions
  - Clears FB on transition to prevent stale DSL pixels persisting under compiled effects
- **Test:** `test_registry_clears_framebuffer_on_effect_switch` — verifies FB is black after switching between partial-coverage effects

### PERF-02: Recursion depth limit + FPS cap + frame timing diagnostics
- **Recursion depth limit** (`src/live/runtime/Executor.cpp`, `include/AppConfig.h`):
  - `evaluateNode()` now has `int16_t depth = 0` parameter
  - Returns `0.0f` (not crash) when depth ≥ `kMaxExpressionDepth` (64)
  - All recursive calls pass `depth + 1`
- **FPS cap** (`src/main.cpp`, `include/AppConfig.h`):
  - `kTargetFrameTimeUs = 16000` (~62.5 FPS) in AppConfig.h
  - After `renderFrame()`, if `g_lastLoopUs < kTargetFrameTimeUs`, delay is added
  - Complex effects that already exceed the target are unaffected
- **Frame timing diagnostics** (`src/main.cpp`, `include/web/StatusJsonBuilder.h`, `src/web/StatusJsonBuilder.cpp`):
  - Ring buffer (64 samples) tracks frame times in `g_frameTimesUs[]`
  - `buildStatusSnapshot()` computes min/max/avg over filled samples
  - `/api/status` JSON includes `minFrameTimeUs`, `maxFrameTimeUs`, `avgFrameTimeUs`
  - Ring buffer resets with the FPS reporting window (every 5s)
- **Tests:**
  - `test_expression_at_depth_limit_succeeds` — 63-deep `sin(...)` evaluates normally
  - `test_deeply_nested_expression_does_not_overflow` — 70-deep is clamped, no crash
  - `test_status_json` — verifies new frame timing fields in JSON output

### PERF-03: Guard serial debug output with APP_IS_DEV
- **Build system** (`platformio.ini`):
  - Added `-D APP_IS_DEV=1` to dev environment build flags
  - Release environment has no APP_IS_DEV define
- **Debug guards** (`src/main.cpp`):
  - Serial heartbeat: `#if APP_IS_DEV` around all `Serial.print()` debug lines
  - `printBootBanner()`: guarded by `#if APP_IS_DEV`
  - Filesystem success prints: guarded by `#if APP_IS_DEV`
  - Error prints (mount fail, etc.) remain unconditional
- **Verification:** Both `esp32-c3-supermini-dev` and `esp32-c3-supermini-release` compile successfully

---

## Files Changed

| File | Changes |
|------|---------|
| `include/AppConfig.h` | +kMaxExpressionDepth, +kTargetFrameTimeUs, +kMinFrameTimeUs |
| `include/effects/EffectRegistry.h` | +requestClear(), +clearRequested_ field |
| `include/web/StatusJsonBuilder.h` | +frameTimeMinUs, +frameTimeMaxUs, +frameTimeAvgUs fields |
| `platformio.ini` | +APP_IS_DEV=1 in dev build flags |
| `src/effects/EffectRegistry.cpp` | clear-on-switch logic in setActiveByName/renderActive |
| `src/live/runtime/Executor.cpp` | depth parameter + guard in evaluateNode, all recursive calls updated |
| `src/main.cpp` | g_wasLiveActive tracking, FPS cap delay, frame timing ring buffer, #if APP_IS_DEV guards |
| `src/web/StatusJsonBuilder.cpp` | Serialize new frame timing fields |
| `test/test_dsl_executor/test_main.cpp` | +2 recursion depth tests |
| `test/test_effect_registry/test_main.cpp` | +FB clear on switch test (SinglePixelEffect) |
| `test/test_status_json/test_main.cpp` | +frame timing field assertions |
| `docs/STATUS.md` | Marked PERF complete, added debug hygiene section |

---

## Test Results

- **New tests:** 5/5 passing
  - `test_registry_clears_framebuffer_on_effect_switch` ✓
  - `test_expression_at_depth_limit_succeeds` ✓
  - `test_deeply_nested_expression_does_not_overflow` ✓
  - `test_status_json` frame timing fields ✓
  - `test_status_json` escape test unchanged ✓

- **Pre-existing failures** (unrelated to Phase 4):
  - `test_framebuffer`: compile error (angleToX→angleToY rename from Phase 2)
  - `test_effects`: compile error (ClockOverlay::render signature changed in Phase 3)
  - `test_live_program_service`: coordinate mismatch (Phase 2 XY swap)
  - `test_dsl_executor` sprite tests: coordinate mismatch (Phase 2 XY swap)

---

## Build Verification

| Environment | Status | RAM | Flash |
|-------------|--------|-----|-------|
| esp32-c3-supermini-dev | ✅ SUCCESS | 15.1% | 89.8% |
| esp32-c3-supermini-release | ✅ SUCCESS | 15.2% | 90.5% |

---

## Visual UAT Checklist (all passed 2026-06-02 on device 192.168.2.119)

- [x] Effect switch via web UI: no ghost pixels from previous effect — **confirmed visually**
- [x] Live→live transition (радуга→огонь→радуга): clean, Executor clears FB each switch
- [x] `/api/status` shows minFrameTimeUs, maxFrameTimeUs, avgFrameTimeUs fields — **confirmed**: min=16587, max=16837, avg=16644 (rainbow)
- [x] Simple effects capped at ~60 FPS (not 500+) — **confirmed**: rainbow FPS=56, loopUs≈16.6ms = kTargetFrameTimeUs
- [x] Release build serial output: no heartbeat, no boot banner — **confirmed**: 0 lines in 12s monitor
- [x] Dev build serial output: heartbeat every 5s — **confirmed**: heartbeat visible in dev monitor
- [x] Complex DSL effects (fire) unaffected by FPS cap — **confirmed**: fire FPS=42, loopUs≈22ms > 16ms cap
- [x] Deep DSL expressions don't crash device (depth clamped at 64) — **confirmed**: unit tests pass

---

## Deferred

| Item | Reason |
|------|--------|
| Clock overlay redraw optimization | Redraws identically 624/625 frames; negligible impact |
| Manual JSON builders → ArduinoJson | Not in render loop; tracked in CONCERNS.md |
| Pre-existing test fixes (test_framebuffer, test_effects, etc.) | Out of scope for PERF phase; coordinate system tests need Phase 2 follow-up |

---

*Completed: 2026-06-02*
