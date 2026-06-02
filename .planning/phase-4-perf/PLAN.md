# PLAN: Phase 4 — Perf & Bugs

**Phase:** 4
**Created:** 2026-06-02
**Status:** Ready
**Requirements:** PERF-01, PERF-02, PERF-03
**Depends on:** Phase 2 (Cylinder)

---

## must_haves

```yaml
truths:
  - "No ghost pixels when switching between compiled effects"
  - "No leftover live-program pixels after stopping DSL program"
  - "Serial output silent in release builds (no heartbeat, no boot banner)"
  - "Deeply nested DSL expressions do not crash device"
  - "Simple effects capped at ~60 FPS; complex effects unaffected"
  - "Frame timing diagnostics (min/max/avg) visible in /api/status"
artifacts:
  - path: "src/effects/EffectRegistry.cpp"
    provides: "FB clear on effect switch via clearRequested_ flag"
  - path: "src/main.cpp"
    provides: "Live→compiled transition clear, debug output guards, FPS cap, frame timing ring buffer"
  - path: "src/live/runtime/Executor.cpp"
    provides: "Recursion depth limit (64 levels) in evaluateNode()"
  - path: "src/web/StatusJsonBuilder.cpp"
    provides: "Serialization of minFrameTimeUs, maxFrameTimeUs, avgFrameTimeUs"
key_links:
  - from: "EffectRegistry::setActiveByName()"
    to: "EffectRegistry::renderActive()"
    via: "clearRequested_ bool flag"
  - from: "LiveProgramService::stop()"
    to: "renderEffectPass()"
    via: "g_wasLiveActive transition detection"
  - from: "Executor::evaluateNode()"
    to: "DSL user input"
    via: "kMaxExpressionDepth = 64 guard"
```

---

## 1. Research Summary

### 1.1 Current State

| Component | Status | Notes |
|-----------|--------|-------|
| `EffectRegistry::renderActive()` | ⚠️ No FB clear | Renders new effect over previous effect's pixels |
| `Executor::render()` | ✅ Clears FB | `frameBuffer.clear()` before rendering layers |
| Live→compiled transition | ⚠️ Stale pixels | `LiveProgramService::stop()` clears program; next frame EffectRegistry draws without clearing |
| Serial heartbeat | ❌ Unconditional | Every 5s prints to Serial at 115200 baud |
| `printBootBanner()` | ❌ Unconditional | Large serial dump at startup — no channel guard |
| Visual heartbeat | ✅ Gone | `AlternatingColumnsEffect` class exists but never instantiated |
| `evaluateNode()` recursion | ❌ No depth limit | Recursive, no guard → potential stack overflow on ESP32-C3 (~4KB stack) |
| Frame rate cap | ❌ None | Main loop runs at max speed; simple effects burn CPU |
| Frame timing diagnostics | ⚠️ Partial | Only current `fps` (5s avg) and `loopUs` (last frame) — no min/max/avg |
| Manual JSON builders | ⚠️ Tech debt | OUT OF SCOPE for this phase (not in render loop) |

### 1.2 Render Pipeline (from RESEARCH.md)

```
loop() → renderFrame(nowMs)
  → renderEffectPass(nowMs)
    → g_liveProgramService.render()   [if live program active: clears FB, renders layers]
    → g_effectRegistry.renderActive()  [fallback: renders compiled effect over EXISTING FB — NO CLEAR]
  → renderOverlayPass(nowMs)
    → g_clockOverlay.render()         [draws clock digits + sensor data over effect]
  → commitFrame()
    → FastLED.show()                  [pushes all 512 pixels to WS2812B]
```

### 1.3 Effect Transition Artifact Scenarios

| From → To | Clear? | Flicker Risk | Fix |
|-----------|--------|-------------|-----|
| Live → Live | ✅ Executor clears | None | — |
| Live → Compiled | ❌ No clear | **High** | Clear FB in `renderEffectPass()` on transition |
| Compiled → Live | ✅ Executor clears | None | — |
| Compiled → Compiled | ❌ No clear | **High** | Clear FB when `EffectRegistry` switches active effect |

**Primary scenario (Live→Compiled):**
1. User stops a live DSL program → `LiveProgramService::stop()` clears state
2. Next frame: `g_liveProgramService.render()` returns false (not active)
3. `g_effectRegistry.renderActive()` renders boot-solid over leftover DSL pixels
4. Since boot-solid is `SolidColorEffect::fill()` (fills all pixels), this specific transition is OK
5. BUT: If default effect were something partial (e.g., AlternatingColumns), artifacts appear

### 1.4 Debug Output Audit

The **visual** heartbeat pattern (alternating boot-solid/debug-columns) no longer exists in the codebase. Only these remain:

| Output | Location | Release Impact |
|--------|----------|---------------|
| Serial heartbeat (every 5s) | `main.cpp:426-435` | Noise on serial at 115200 baud |
| `printBootBanner()` | `main.cpp:305-354` | ~50 lines of debug info at boot |
| `Serial.println()` in `initializeFileSystem()` | `main.cpp:148-164` | Mount errors are fine; success prints are noise |

All are **unconditional** — no `#if APP_CHANNEL == "dev"` guards. The build system has two environments:
- `esp32-c3-supermini-dev` → `APP_CHANNEL=dev`
- `esp32-c3-supermini-release` → `APP_CHANNEL=stable`

### 1.5 Recursion Depth Risk

`Executor::evaluateNode()` (Executor.cpp:32-105) is fully recursive with no depth limit:
- Binary ops (`kAdd`, `kMultiply`): 2 recursive calls per node
- Unary ops (`kSin`, `kCos`, `kNegate`): 1 recursive call
- Ternary ops (`kClamp`, `kMix`, `kSmoothstep`): 3 recursive calls

On ESP32-C3 (~4KB per-task stack), deeply nested expressions like `sin(sin(sin(...sin(t)...)))` could overflow. This manifests as a device hang/crash — perceived as a frame drop.

**Current mitigation:** None. DSL parser doesn't enforce depth either.

### 1.6 Frame Timing

Current FPS: `g_frameCount * 1000 / (millis() - g_lastFpsReportMs)` over 5s windows. Resets counter every window.
Current loop time: `g_lastLoopUs = micros() - loopStart` — only last frame.

**Missing:** No min/max/avg frame time tracking for diagnostics. No frame rate cap. No animation delta-time consistency.

---

## 2. Tasks

### Task 1: Clear framebuffer on effect transition (PERF-01)

**Files:** `src/main.cpp`, `include/effects/EffectRegistry.h`, `src/effects/EffectRegistry.cpp`

**Changes:**

1. **EffectRegistry: Add `clearOnSwitch()` method**
   - Add `bool clearRequested_ = false` private field
   - Add `void requestClear()` — sets `clearRequested_ = true`
   - Modify `renderActive()`: if `clearRequested_` is true, call `context.frameBuffer.clear()` before `active_->render(context)`, then reset `clearRequested_ = false`
   - Modify `setActiveByName()`: when successfully switching to a different effect, call `requestClear()`

2. **main.cpp: Detect live→compiled transition and clear FB**
   - In `renderEffectPass()`: track whether the previous frame was a live program render
   - Add `static bool s_wasLiveActive = false` (or file-scope `g_wasLiveActive`)
   - When `g_liveProgramService.render()` returns false BUT `s_wasLiveActive` was true: call `g_frameBuffer.clear()` before falling through to `EffectRegistry::renderActive()`
   - Update `s_wasLiveActive` after each render decision

**Verification:**
- Unit test: Switch EffectRegistry between two effects — verify FB is cleared between them
- Unit test: Simulate live→compiled transition — verify FB is cleared
- Visual UAT: Rapidly switch effects via web UI; no ghost pixels from previous effect

**Done:** `renderEffectPass()` clears FB on live→compiled transition AND `EffectRegistry::renderActive()` clears on effect switch — verified by unit tests passing + clean visual UAT.

---

### Task 2: Guard serial debug output with APP_CHANNEL (PERF-03)

**Files:** `src/main.cpp`

**Changes:**

1. **Guard serial heartbeat with `#if`**
   - Wrap the entire heartbeat block (`main.cpp:426-435`) in `#if APP_CHANNEL == "dev"`
   - Keep the `g_webServer.setStatusSnapshot(buildStatusSnapshot())` call OUTSIDE the guard (it's functional, not debug)

2. **Guard `printBootBanner()` with channel check**
   - Wrap `printBootBanner()` call at end of `setup()` in `#if APP_CHANNEL == "dev"`

3. **Guard filesystem success prints**
   - In `initializeFileSystem()`: wrap `Serial.print("filesystem: mounted...")` in `#if APP_CHANNEL == "dev"`
   - Keep `Serial.println("filesystem: mount failed")` and error prints unconditional (they indicate real problems)

4. **Verify release build produces clean serial output**
   - Build with `esp32-c3-supermini-release` environment
   - Check: no "heartbeat", no "mylamp bootstrap" banner, no "filesystem: mounted presets="

**Verification:**
- Build both dev and release; flash release; verify serial output is silent (no heartbeat, no boot banner)
- Dev build still prints heartbeat and boot banner
- Run `platformio run --environment esp32-c3-supermini-release` — compiles without error

**Done:** Release build (`esp32-c3-supermini-release`) compiles and produces zero debug lines (no heartbeat, no boot banner) during boot and 30s runtime. Dev build still emits debug output.

---

### Task 3: Add recursion depth limit to evaluateNode (PERF-02)

**Files:** `src/live/runtime/Executor.cpp`

**Changes:**

1. **Add depth tracking to `evaluateNode()`**
   - Add `static constexpr int16_t kMaxExpressionDepth = 64`
   - Add `int16_t depth` parameter to `evaluateNode()` (default 0)
   - At function entry: `if (depth >= kMaxExpressionDepth) return 0.0f;`
   - Pass `depth + 1` in all recursive calls

2. **Update all call sites**
   - `evaluateColor()` — pass `depth = 0` initially
   - `renderSpritePixel()` calls `evaluateColor()` which calls `evaluateNode()` — chain is fine
   - The `render()` method calls `evaluateNode()` for layer-level expressions (visible, x, y, scale, rotation) — pass `0` for depth at each top-level call

3. **Add compile-time constant to AppConfig (optional)**
   - Add `static constexpr int16_t kMaxExpressionDepth = 64;` to `lamp::config` in `AppConfig.h`
   - Reference it in Executor.cpp instead of hardcoding

**Verification:**
- Unit test: DSL expression with 100 nested sin() calls — should return 0.0f (clamped) instead of crashing
- Unit test: Expression at depth 63 succeeds, depth 64 returns 0.0f
- Existing DSL tests still pass (none approach depth 64)

**Done:** `evaluateNode()` returns 0.0f (not crash) when expression depth exceeds 64. Existing DSL tests pass unchanged. Unit test confirms depth clamping behavior.

---

### Task 4: Add configurable frame rate cap (PERF-02)

**Files:** `include/AppConfig.h`, `src/main.cpp`

**Changes:**

1. **Add FPS target constant to AppConfig**
   - Add `static constexpr uint32_t kTargetFrameTimeUs = 16000;` (~62.5 FPS) to `lamp::config`
   - Add `static constexpr uint32_t kMinFrameTimeUs = 8000;` (~125 FPS) as absolute floor

2. **Add frame delay in main loop**
   - In `loop()`, after `renderFrame(now)` and `g_lastLoopUs` computation:
   - `if (g_lastLoopUs < kTargetFrameTimeUs) { delayMicroseconds(kTargetFrameTimeUs - g_lastLoopUs); }`
   - This caps FPS at ~62.5, preventing CPU waste on simple effects
   - For complex effects that already exceed the target (e.g., 40ms = 25 FPS), no delay is added

3. **Use actual elapsed time for delta calculations**
   - The `renderEffectPass()` receives `nowMs` (millis) — unaffected by frame delay
   - The delta for DSL animations already comes from `nowMs - g_lastRenderMs` — correct
   - Ensure `g_lastLoopUs` reflects actual render + delay time for accurate status reporting

**Verification:**
- Flash dev build; run simple SolidColor effect; observe FPS in `/api/status` — should be ~60 (not 500+)
- Run complex DSL effect (fireplace); FPS should be unchanged (below cap) — no artificial slowdown
- `loopUs` field in status reflects total loop time including optional delay

**Done:** FPS is capped at ~60 for simple effects. Complex DSL effects (below cap) run at native speed unaffected. `kTargetFrameTimeUs = 16000` in AppConfig.h is single point of adjustment.

---

### Task 5: Add frame timing diagnostics to status (PERF-02)

**Files:** `include/web/StatusJsonBuilder.h`, `src/web/StatusJsonBuilder.cpp`, `src/main.cpp`

**Changes:**

1. **Add fields to `StatusSnapshot`**
   - Add `uint32_t frameTimeMinUs = 0;` — minimum frame time in current window
   - Add `uint32_t frameTimeMaxUs = 0;` — maximum frame time in current window
   - Add `uint32_t frameTimeAvgUs = 0;` — average frame time in current window

2. **Track rolling frame times in main.cpp**
   - Add `static constexpr size_t kFrameTimeWindow = 64;` — rolling window size
   - Add `uint32_t g_frameTimesUs[kFrameTimeWindow]` ring buffer
   - Add `size_t g_frameTimeIndex = 0`
   - In `loop()`, after computing `g_lastLoopUs`: store in ring buffer at `g_frameTimeIndex`, advance index (mod kFrameTimeWindow)
   - Compute min/max/avg over filled portion of ring buffer when building status snapshot

3. **Serialize new fields in StatusJsonBuilder**
   - Add `minFrameTimeUs`, `maxFrameTimeUs`, `avgFrameTimeUs` to JSON output
   - Use existing `appendFloatField` pattern — consistent with `fps` and `loopUs`

4. **Reset frame time window with FPS window**
   - When `g_lastFpsReportMs` resets (every 5s), also reset the frame time ring buffer index to 0
   - This keeps frame time stats aligned with the FPS reporting window

**Verification:**
- Unit test: `buildStatusJson()` includes new fields with valid values
- Visual: Check `/api/status` — see `minFrameTimeUs`, `maxFrameTimeUs`, `avgFrameTimeUs`
- Frame times are consistent: min ≤ avg ≤ max for any given snapshot

**Done:** `/api/status` JSON includes `minFrameTimeUs`, `maxFrameTimeUs`, `avgFrameTimeUs` fields. Values are valid (non-zero after frames have rendered). Ring buffer (64 samples) reflects most recent ~1 second of frames.

---

### Task 6: Add/Update tests (PERF-01, PERF-02, PERF-03)

**Files:**
- `test/test_effect_registry/test_main.cpp`
- `test/test_live_program_service/test_main.cpp`
- `test/test_dsl_executor/test_main.cpp`
- `test/test_status_json/test_main.cpp`
- `test/test_framebuffer/test_main.cpp`

**Changes:**

1. **test_effect_registry: Add FB clear on switch test**
   - `test_registry_clears_framebuffer_on_effect_switch()`:
     - Register two effects
     - Render first (fills some pixels), switch to second, render
     - Verify pixels NOT filled by second effect are black (0,0,0) — proof of clear

2. **test_live_program_service: Add stop→clear transition test**
   - `test_stop_leaves_framebuffer_ready_for_compiled_effect()`:
     - Run live program, stop it, render compiled effect in same FB
     - Verify no stale live pixels remain where compiled effect doesn't draw

3. **test_dsl_executor: Add recursion depth limit test**
   - `test_deeply_nested_expression_does_not_overflow()`:
     - Compile a DSL program with `sin(sin(sin(...sin(t)...)))` nested 70 deep
     - Execute render — should not crash (depth clamped at 64, returns 0 for overflow)
   - `test_expression_at_depth_limit_succeeds()`:
     - Expression at exactly depth 63 — should evaluate normally

4. **test_status_json: Add frame timing fields test**
   - Extend `test_status_json_contains_build_and_runtime_fields()`:
     - Set `frameTimeMinUs`, `frameTimeMaxUs`, `frameTimeAvgUs` in snapshot
     - Verify they appear in JSON output

5. **test_framebuffer: Verify clear() fills all pixels black**
   - Existing test likely covers this; if not, add `test_clear_sets_all_512_pixels_to_black()`

**Verification:**
- Run all tests with `platformio test --environment native-test`
- All existing tests continue to pass (no regressions)
- New tests pass: effect switch clear, recursion depth limit, frame timing fields

**Done:** `platformio test --environment native-test` passes with 0 failures. New tests cover: FB clear on effect switch, live→compiled transition, recursion depth clamping, frame timing JSON fields, and FB clear integrity.

---

### Task 7: Update docs

**File:** `docs/STATUS.md`

**Changes:**
1. Mark PERF-01, PERF-02, PERF-03 as complete
2. Update known issues section: remove "visual heartbeat" if listed, note that serial debug is now channel-gated
3. Add note about frame rate cap (kTargetFrameTimeUs = 16000) and where to adjust it
4. Add note about recursion depth limit (kMaxExpressionDepth = 64) for DSL authors

---

## 3. Verification Plan

### Unit Tests (native)
```bash
platformio test --environment native-test
```
- `test_effect_registry` — FB clear on effect switch
- `test_live_program_service` — stop→clear transition
- `test_dsl_executor` — recursion depth limit (clamped, not crashed)
- `test_status_json` — frame timing fields in JSON
- `test_framebuffer` — clear all pixels

### Build Verification
```bash
# Release build compiles without APP_CHANNEL-guarded debug code
platformio run --environment esp32-c3-supermini-release
```

### Visual UAT (on device)

**Test A: Effect Switch Flicker (PERF-01)**
1. Flash dev build
2. Open web UI → start a complex DSL effect (e.g., fireplace)
3. Stop the live program → lamp falls back to boot-solid
4. **Expected:** Clean transition — no ghost pixels from fireplace remain
5. Switch compiled effects rapidly via API: `POST /api/effect` with different names
6. **Expected:** Each switch shows clean effect start — no blend of old+new

**Test B: Frame Rate Stability (PERF-02)**
1. Run fireplace DSL effect (moderate complexity)
2. Check `/api/status` → observe `fps` field
3. **Expected:** FPS ≥ 20 for fireplace; frameTimeMin/Max/Avg are reasonable (not 0, not wildly varying)
4. Switch to simple SolidColor effect
5. **Expected:** FPS capped at ~60 (not 500+); CPU not pegged

**Test C: Release Serial Silence (PERF-03)**
1. Build and flash release (`esp32-c3-supermini-release`)
2. Open serial monitor at 115200 baud
3. **Expected:** No "heartbeat", no "mylamp bootstrap" banner
4. Error messages (mount fail, etc.) still appear if they occur

### Nyquist Validation
Each requirement must have ≥1 test:
- **PERF-01:** `test_effect_registry` (FB clear on switch) + `test_live_program_service` (stop→clear) + Visual UAT A
- **PERF-02:** `test_dsl_executor` (recursion depth) + `test_status_json` (frame timing fields) + Visual UAT B
- **PERF-03:** Release build compilation + Visual UAT C (serial silence)

---

## 4. Risk Analysis

| Risk | Severity | Mitigation |
|------|----------|------------|
| FB clear causes brief black flash between effects | Low | Clear is instant (512-byte memset); no visible flicker at 60+ FPS |
| Recursion depth limit changes DSL semantics | Low | Depth 64 is far deeper than any realistic DSL expression; users never hit it |
| Frame rate cap disrupts animation timing | Low | DSL uses wall-clock `nowMs`, not frame count; animation speed unaffected |
| Release build breaks due to `#if` guards | Low | Both dev and release build verified in CI (platformio run for both envs) |
| Ring buffer for frame times uses RAM | Low | 64 × 4 = 256 bytes; well within ESP32-C3 budget |
| Missed debug output in release complicates field debugging | Medium | Keep error/failure prints unconditional; only suppress periodic heartbeat and boot banner |

---

## 5. Task Execution Order

```
Task 1 (FB clear on switch) ──┐
Task 2 (debug guard)        ──┼── Wave 1: Independent, different files
Task 3 (recursion depth)    ──┘

Task 4 (FPS cap) ──┐
                   ├── Wave 2: Both touch main.cpp loop, sequential within wave
Task 5 (frame diag)──┘         Task 4 first (adds delay), Task 5 second (tracks timing)

Task 6 (tests) ── Wave 3: After all implementation tasks

Task 7 (docs) ── Wave 4: After all tests pass
```

**Parallel notes:**
- Tasks 1, 2, 3 can run in parallel (no shared files)
- Task 4 must precede Task 5 (Task 5 tracks `loopUs` which includes Task 4's delay)
- Task 6 depends on Tasks 1-5 (tests for each change)
- Task 7 is documentation — last

---

## 6. Threat Model

### Trust Boundaries

| Boundary | Description |
|----------|-------------|
| Serial output → external monitor | Debug info could leak device state in release |
| DSL source → expression evaluator | Untrusted user input (DSL code from web UI) enters recursive evaluator |
| Web UI → effect switching API | User-triggered effect transitions must not cause visual instability |

### STRIDE Threat Register

| Threat ID | Category | Component | Disposition | Mitigation Plan |
|-----------|----------|-----------|-------------|-----------------|
| T-PERF-01 | Denial of Service | `Executor::evaluateNode()` | **mitigate** | Recursion depth limit (64) prevents stack overflow from maliciously deep DSL expressions |
| T-PERF-02 | Information Disclosure | Serial heartbeat (`main.cpp`) | **mitigate** | `#if APP_CHANNEL == "dev"` guards on serial debug; release builds produce no periodic output |
| T-PERF-03 | Information Disclosure | `printBootBanner()` (`main.cpp`) | **mitigate** | Gated on `APP_CHANNEL == "dev"`; release builds skip boot banner |
| T-PERF-04 | Denial of Service | Main loop (`main.cpp`) | **accept** | No frame rate cap could burn CPU on simple effects, but FastLED.show() provides natural backpressure (~30μs per pixel × 512 = ~15ms minimum). Risk of device overheating is low at kDefaultBrightness=32. Frame cap (Task 4) further mitigates. |

---

## 7. Success Criteria

- [ ] `EffectRegistry::renderActive()` clears FB when active effect changes — no ghost pixels
- [ ] `renderEffectPass()` detects live→compiled transition and clears FB
- [ ] Serial heartbeat and boot banner are absent in `esp32-c3-supermini-release` builds
- [ ] `evaluateNode()` returns 0.0f (not crash) when expression depth exceeds 64
- [ ] Frame rate is capped at ~62.5 FPS for simple effects; complex effects unaffected
- [ ] `/api/status` JSON includes `minFrameTimeUs`, `maxFrameTimeUs`, `avgFrameTimeUs`
- [ ] All existing tests pass; new tests for PERF-01/02/03 pass
- [ ] Visual UAT confirms no flicker during effect transitions on real device

**Done:** `docs/STATUS.md` reflects PERF-01/02/03 completion. Known issues section updated. Frame rate cap and recursion depth limit documented with adjustment pointers.

---

## 8. Deferred

| Item | Reason |
|------|--------|
| Clock overlay redraw optimization | `ClockOverlay::render()` redraws identically 624/625 frames (rotation offset changes every 625ms). Impact negligible — ~50-100 `setPixel` calls per frame. Candidate for future micro-optimization phase. |
| Manual JSON builders → ArduinoJson migration | `buildUpdateSettingsJson`, `buildCurrentUpdateJson`, `buildCheckUpdatesJson` use manual string concatenation. Not in render loop — negligible performance impact. Separately tracked as tech debt in CONCERNS.md. |

---

*Created: 2026-06-02*
*Plan type: execute*

## PLANNING COMPLETE
