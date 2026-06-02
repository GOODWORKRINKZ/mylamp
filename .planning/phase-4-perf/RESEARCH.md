# Phase 4: PERF — Research

**Phase:** 4 — Баги и производительность
**Researched:** 2026-06-02
**Status:** Complete

---

## 1. Current State Audit

### 1.1 Render Pipeline

```
loop() → renderFrame(nowMs)
  → renderEffectPass(nowMs)
    → g_liveProgramService.render()   [if live program active: clears FB, renders layers]
    → g_effectRegistry.renderActive()  [fallback: renders active compiled effect over existing FB]
  → renderOverlayPass(nowMs)
    → g_clockOverlay.render()         [draws clock digits + sensor data over effect]
  → commitFrame()
    → FastLED.show()                  [pushes all 512 pixels to WS2812B]
```

**Key observation:** `renderEffectPass` does NOT clear the framebuffer between effect switches when using compiled effects. `LiveProgramService::render()` calls `frameBuffer.clear()` internally, but `EffectRegistry::renderActive()` just calls `effect->render()` — which may or may not fill all pixels.

### 1.2 Effect Transition Behavior

| From → To | Clear? | Flicker Risk |
|-----------|--------|-------------|
| Live program → Live program | Yes (Executor::render clears FB first) | None |
| Live program → Compiled effect | No (LiveProgramService::stop() → next frame: EffectRegistry) | High |
| Compiled effect → Live program | Yes (Executor clears FB) | None |
| Compiled effect → Compiled effect | No (EffectRegistry just switches pointer) | High |

**Specific scenario producing artifacts:**
1. User stops a live DSL program
2. Next frame: `g_liveProgramService.render()` returns false
3. `g_effectRegistry.renderActive()` renders boot-solid over whatever pixels remained from the DSL program
4. Since boot-solid is `SolidColorEffect::fill()` it covers everything → no flicker in this case
5. BUT: if the active compiled effect is `AlternatingColumnsEffect` (not currently registered), only alternating columns are drawn — previous frame's pixels remain visible in other columns

### 1.3 Debug Heartbeat

**Current state:** The visual heartbeat (alternating boot-solid ↔ debug-columns) described in CONCERNS.md lines 370-392 **no longer exists** in the codebase. Only a serial heartbeat remains:

```cpp
// main.cpp:426-435 — Serial heartbeat every 5s
if (now - g_lastHeartbeatMs >= 5000UL) {
    g_lastHeartbeatMs = now;
    Serial.print("heartbeat uptime_ms=");
    Serial.println(now);
    // ... status lines ...
}
```

The `AlternatingColumnsEffect` class exists but is **never instantiated** in main.cpp — only `#include`d. The debug-columns pattern was apparently removed before Phase 2.

**Remaining debug artifacts in release:**
- Serial heartbeat (harmless but noisy)
- `printBootBanner()` — large serial dump at startup (lines 305-354)
- No `#if APP_CHANNEL == "dev"` guards anywhere

### 1.4 DSL Expression Evaluation — Recursion Risk

`Executor::evaluateNode()` (Executor.cpp:32-105) is recursive with **no depth limit**. Each recursive call consumes stack space. On ESP32-C3 with ~4KB per-task stack:

- Binary ops (`kAdd`, `kMultiply`, etc.): 2 recursive calls → tree depth = expression tree height
- Trigonometric (`kSin`, `kCos`): 1 recursive call
- Ternary (`kClamp`, `kMix`, `kSmoothstep`): 3 recursive calls

**Worst case:** `sin(sin(sin(...sin(t)...)))` nested 30+ deep could overflow.

**Current mitigation:** None. The DSL parser doesn't enforce depth limits either.

### 1.5 FPS / Frame Timing

**Current FPS measurement:** Counted per-frame, reported every 5 seconds via status JSON. Formula:
```cpp
fps = g_frameCount * 1000 / (millis() - g_lastFpsReportMs)
```
After 5s window, `g_frameCount` resets to 0. This is correct but produces a **lagging** value in `/api/status` — the snapshot captures whatever the running average is at request time.

**No frame rate cap:** Main loop runs at max speed. For simple effects:
- `SolidColorEffect::fill()` → ~500-1000 FPS (just memory fill + FastLED.show)
- Complex DSL effects → 10-40 FPS (per-pixel expression evaluation)
- Inconsistent timing causes visible jitter in animations

### 1.6 Manual JSON String Building

Three functions build JSON via repeated `std::string +=`:
- `buildUpdateSettingsJson()` — 3 fields, called on GET/POST `/api/settings/update`
- `buildCurrentUpdateJson()` — 8 fields, called on GET `/api/status/update`
- `buildCheckUpdatesJson()` — 7 fields, called on POST `/api/update/check`

**Performance impact:** Negligible (called once per API request, not in render loop). **Quality impact:** Inconsistent with `StatusJsonBuilder` which uses ArduinoJson. Manual escaping is fragile.

### 1.7 Clock Overlay Redraw

`ClockOverlay::render()` redraws all clock digits + sensor line every frame. The rotation offset changes at 1px every 625ms (3 rpm = 20s period / 32 pixels). So 624 out of 625 frames produce identical output. This is wasteful but not a bug — it's a design choice trading CPU for simplicity.

---

## 2. Root Causes of Visual Artifacts

### 2.1 Primary: No FrameBuffer Clear on Effect Switch

When `EffectRegistry::setActiveByName()` switches effects, the frame buffer still contains the previous effect's pixels. The new effect renders on top. If the new effect doesn't fill every pixel, old pixels persist.

**Fix:** Clear framebuffer when active effect changes. Add `clearOnSwitch` flag to EffectRegistry, or clear in `renderEffectPass()` when live program transitions to compiled effect.

### 2.2 Secondary: DSL Program Stop → Boot Effect Transition

Sequence:
1. Live program active → `Executor::render()` clears FB then draws
2. User stops live program → `LiveProgramService::stop()` clears `activeProgram_`
3. Next frame: `renderEffectPass()` → live program not active → falls through to `EffectRegistry::renderActive()`
4. Boot effect renders over last DSL frame's pixels

Since boot effect is `SolidColorEffect::fill()` (fills all pixels), this specific transition is actually fine. But if the default effect were something else (e.g., AlternatingColumns), artifacts would appear.

**Fix:** Guarantee that the default/boot effect always fills all pixels, OR clear FB in `renderEffectPass()` when transitioning away from a live program.

### 2.3 Clock Overlay Flicker

Clock overlay uses `setPixel()` to draw digits and sensor data. On each frame it redraws. If the overlay position overlaps with bright effect pixels, there may be brief flicker as the overlay draws pixel-by-pixel on top. Not currently a problem because the effect renders first, then the overlay draws on top — but if frame timing causes a FastLED.show() mid-overlay (not possible with current single-threaded code), flicker would occur.

---

## 3. Performance Bottlenecks

| Bottleneck | Severity | Location | Impact |
|-----------|----------|----------|--------|
| No frame rate cap | Low | main.cpp:438-440 | Unnecessary CPU usage for simple effects |
| Full FB clear every DSL frame | Low | Executor.cpp:205 | 512-byte memset per frame |
| Clock overlay full redraw every frame | Low | ClockOverlay.cpp:95-152 | ~50-100 setPixel calls per frame |
| Manual JSON string building | Negligible | LampWebServer.cpp:79-111 | Infrequent API calls |
| Recursive expression eval (no depth limit) | Medium | Executor.cpp:32-105 | Potential stack overflow |
| No FastLED brightness optimization | None | N/A | Already set to 32/255 |

---

## 4. What PERF-01, PERF-02, PERF-03 Mean Concretely

### PERF-01: Устранено мерцание и визуальные артефакты

**What to fix:**
1. Ensure framebuffer is cleared or fully covered when switching between effects
2. Verify no partial-frame artifacts during effect transitions
3. Test: switch effects rapidly via web UI, observe LED matrix for flashes

### PERF-02: Стабильный фреймрейт, без заметных просадок

**What to fix:**
1. Add recursion depth limit to `evaluateNode()` (prevents crashes that could look like frame drops)
2. Consider adding a minimum frame time (e.g., 16ms = ~60fps cap) for consistent animation timing
3. Add frame timing diagnostics to status: min/max/avg frame time over last N frames
4. Test: run complex DSL effect (fireplace) and verify FPS stays above acceptable threshold (e.g., 20+)

### PERF-03: Убран дебаг-хартбит в релизных сборках

**What to fix:**
1. Guard serial heartbeat with `#if APP_CHANNEL == "dev"` (or `#ifdef DEBUG_HEARTBEAT`)
2. Guard `printBootBanner()` with channel check or verbosity flag
3. Ensure `esp32-c3-supermini-release` environment produces clean output
4. (Already done) The visual heartbeat pattern no longer exists — verify this

---

## 5. Existing Test Coverage (Relevant to PERF)

| Test | Coverage | PERF Relevance |
|------|----------|---------------|
| `test_framebuffer` | setPixel, clear, fill, fillRect, drawLine, drawCircle | Verifies fill covers all pixels |
| `test_effects` | SolidColorEffect, ClockOverlay | Verifies effects render correctly |
| `test_dsl_executor` | Sprite rendering, layer order, animation | Verifies DSL clear+render pipeline |
| `test_live_program_service` | activate/stop transitions | Could be extended for stop→clear test |

**Gaps:** No test for effect switching artifacts, no test for recursion depth enforcement, no test for channel-dependent debug output.

---

## 6. Implementation Approach

### Approach A: Minimal (fix only clear bugs)
- Add FB clear on effect switch in EffectRegistry
- Add `#if APP_CHANNEL == "dev"` guards on serial debug output
- Add recursion depth limit to evaluateNode
- **Effort:** Small (~30 lines changed)
- **Risk:** Low

### Approach B: Robust (add frame timing + diagnostics)
- Approach A +
- Add frame time tracking (min/max/avg over window)
- Add configurable FPS target with frame delay
- Add effect transition smoothing (crossfade or delayed clear)
- **Effort:** Medium (~100 lines changed)
- **Risk:** Low-Medium

### Approach C: Comprehensive (refactor JSON + add tests)
- Approach B +
- Migrate manual JSON builders to ArduinoJson
- Add PERF-specific unit tests
- Add frame timing to status API
- **Effort:** Large (~200+ lines changed)
- **Risk:** Medium (ArduinoJson migration may introduce regressions)

**Recommendation:** Approach B — fixes the real bugs, adds diagnostics, doesn't over-reach into JSON refactoring (which is tech debt, not a performance bug).

---

## 7. Decisions to Lock

| Decision | Options | Recommendation |
|----------|---------|---------------|
| Effect transition clear strategy | Clear FB on every switch vs. require effects to fill all pixels | Clear on switch (simpler, safer) |
| Frame rate cap | No cap vs. configurable cap vs. fixed 60fps cap | Configurable cap (default 60fps) via config constant |
| Recursion depth limit | 32 vs. 64 vs. 128 levels | 64 levels (covers sin(sin(...)) up to reasonable depth) |
| Debug output guard | `#if APP_CHANNEL` vs. runtime flag vs. remove entirely | `#if APP_CHANNEL == "dev"` (standard pattern) |
| JSON migration scope | This phase vs. separate tech-debt phase | Separate phase (not PERF-critical) |

---

*Research complete. Ready for planning.*
