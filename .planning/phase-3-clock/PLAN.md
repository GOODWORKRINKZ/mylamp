# PLAN: Phase 3 — Clock Overlay

**Phase:** 3
**Created:** 2026-06-02
**Status:** Ready
**Requirements:** CLOCK-01, CLOCK-02, CLOCK-03
**Depends on:** Phase 1 (NTP), Phase 2 (Cylinder)

---

## 1. Research Summary

### 1.1 Current State

| Component | Status | Notes |
|-----------|--------|-------|
| `ClockOverlay::render()` | ⚠️ Old coords | Uses `kLogicalWidth=32` positioning — broken after XY-swap |
| `g_timeState.clockOverlayVisible` | ✅ Works | Set by TimePlanner when NTP synced |
| NTP sync | ✅ Works | Phase 1 — HTTP Date header |
| 12h/24h toggle | ❌ Missing | No UI, no setting |
| Cylinder geometry | ✅ Works | Phase 2 — XY-swap validated |

### 1.2 What Needs Fixing

**ClockOverlay broken after XY-swap:**
- Old code: `originX = kLogicalWidth(32) - 6 - 1 = 25` → now `16 - 6 - 1 = 9` (wrong position)
- Old code: digits drawn along Y axis → now Y=circumference, digits go HORIZONTAL
- Need: digits along X (height), positioned at right side of cylinder

**Missing 12h/24h toggle:**
- No settings field for clock format
- No web UI control
- No API endpoint

### 1.3 Clock Rendering Design (post-swap)

```
     Y=0                         Y=31
     ┌─────────────────────────────┐  X=0 (top)
     │                             │
     │                    12:34    │  X=1
     │                     ██ ██   │
     │                    ████     │
     │                             │
     └─────────────────────────────┘  X=15 (bottom)
```

- Position: `originY = 32 - 6 - 1 = 25`, `originX = 1`
- Digits: binary columns along X (0..3), 4px tall
- Separator: colon at x=1..2 between digits
- Format: HH:MM (5 chars)

---

## 2. Tasks

### Task 1: Fix ClockOverlay for XY-swap

**File:** `src/effects/ClockOverlay.cpp`

**Changes:**
1. Reposition: `originY = kLogicalHeight - kOverlayWidth - margin` (32-6-1=25)
2. `originX = kOverlayMarginTop` (1)
3. Swap digit rendering: `setPixel(originX + dy, originY + col, color)` instead of `setPixel(x, y + dy)`
4. Fix separator: `setPixel(originX + 1, originY + 2)` and `setPixel(originX + 2, originY + 2)`

### Task 2: Add 12h/24h format setting

**Files:** `include/settings/AppSettings.h`, `include/web/TimeSettingsJson.h/.cpp`

**Changes:**
1. Add `bool clock24h = true` to `ClockSettings` struct
2. Add `clock24h` to time settings JSON (GET returns it, POST accepts it)
3. Persist to NVS

### Task 3: Add 12h/24h conversion logic

**File:** `src/time/TimeRuntimeService.cpp` or `src/effects/ClockOverlay.cpp`

**Changes:**
1. If `clock24h = false`, convert 24h time to 12h before rendering
2. `13:00:00` → `01:00:00`, `00:30:00` → `12:30:00`

### Task 4: Add 12h/24h toggle to web UI

**File:** `frontend/src/main.ts`

**Changes:**
1. Add checkbox/switch in time modal: "24-часовой формат"
2. Read/write via `/api/settings/time`

### Task 5: Update tests

**Files:** `test/test_effects/test_main.cpp`, `test/test_time_settings_api/test_main.cpp`

**Changes:**
1. ClockOverlay test: verify correct position with XY-swap
2. Time settings test: verify clock24h field

### Task 6: Update docs

**File:** `docs/STATUS.md`

---

## 3. Verification

### Unit Tests
- ClockOverlay renders at correct position (not clipped)
- 12h/24h conversion: `13:00` → `01:00` PM, `00:00` → `12:00` AM
- Time settings include `clock24h`

### Visual UAT (on lamp)
```lux
effect "clock_uat"

sprite bar {
  bitmap """
################################
################################
  """
}

layer bg {
  use bar
  color hsv(t * 30, 0.5, 1)
  x = 0
  y = 0
  scale = 1
  visible = 1
}
```
**Expected:** Coloured background fills matrix. Clock digits visible on top, not distorted.
Toggle 12h/24h in UI → clock format changes on lamp.

---

## 4. Task Execution Order

```
Task 1 (fix overlay) ──→ Task 3 (12h/24h logic)
Task 2 (settings) ──→ Task 4 (UI toggle)
Task 5 (tests) ── after Tasks 1-4
Task 6 (docs) ── after all
```
