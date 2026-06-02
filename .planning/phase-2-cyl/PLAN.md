# PLAN: Phase 2 — Cylindrical Geometry

**Phase:** 2
**Created:** 2026-06-02
**Status:** Ready
**Requirements:** CYL-01, CYL-02, CYL-03, CYL-04, CYL-05, CYL-06
**Depends on:** None

---

## 1. Research Summary

### 1.1 Current State

| Component | Status | Notes |
|-----------|--------|-------|
| `MatrixLayout::wrapX()` | ✅ Exists | X-coordinates already wrap modulo 32 |
| `MatrixLayout::toLinearIndex()` | ✅ Works | Maps (x,y) → LED index with serpentine correction |
| `FrameBuffer::setPixel()` | ✅ Wraps X | Uses `wrapX()` automatically |
| `FrameBuffer::fill()` | ✅ Works | Fills all 512 pixels |
| `FrameBuffer::clear()` | ✅ Works | Sets all pixels to black |
| `fillRect` / `drawLine` | ❌ Missing | No shape drawing primitives |
| Angle-based API | ❌ Missing | No `angleToX(degrees)` helper |
| Compiled effects (4) | ✅ Work | Use `setPixel(x,y)` — automatically wrap X |
| DSL executor | ✅ Works | Iterates pixel-by-pixel, uses wrapped X |
| `test_framebuffer` | ⚠️ Partial | Only tests `setPixel` wrapping and `clear` |
| `test_effects` | ⚠️ Partial | Only tests SolidColor + ClockOverlay |
| `test_dsl_executor` | ✅ Good | Tests sprite rendering, layer order, animation |

### 1.2 What's Already Cylindrical

The physical matrix is **already cylindrical** in a practical sense:
- 2 panels of 16×16 arranged in a horizontal strip (32×16 logical)
- `wrapX()` makes the left edge (x=0) neighbor the right edge (x=31)
- `setPixel(32, y)` writes to `(0, y)` — wraparound works

The hardware is physically wrapped around a cylinder. A pixel at x=0 and a pixel at x=31 are physically adjacent on the cylinder surface. The software already respects this via `wrapX()`.

### 1.3 What's Missing

1. **Drawing primitives** (`fillRect`, `drawLine`, `drawCircle`) don't exist. Effects render pixel-by-pixel directly. Adding these enables richer effects and proper wraparound-aware shape rendering.

2. **Angle-based coordinate API** — no `MatrixLayout::angleToX(float degrees)` helper. Effects manually compute `x = angle / 360.0f * 32`.

3. **fillRect wraparound** — if `fillRect(30, 0, 5, 16)` is called (rect spanning from x=30 to x=34), it should wrap and fill columns 30,31,0,1,2. Current pixel-by-pixel approach handles this automatically, but a dedicated `fillRect` with internal wrapping would be more efficient.

4. **Test coverage** — no tests for wraparound edge cases (rect spanning boundary, line crossing boundary, circle at boundary).

5. **Documentation** — no formal spec for the cylindrical coordinate system.

---

## 2. Tasks

### Task 1: Add angle-based coordinate helpers to `MatrixLayout`

**Files:** `include/MatrixLayout.h`, `src/MatrixLayout.cpp`

**Changes:**
1. Add `static constexpr float kCircumference = 32.0f;`
2. Add `angleToX(float degrees)` — maps 0°→x=0, 360°→x=32(wraps to 0)
3. Add `xToAngle(uint8_t x)` — maps x→degrees for debugging
4. Add `rowCount()` and `colCount()` accessors (semantic aliases for height/width)

### Task 2: Add `fillRect` to `FrameBuffer` with wraparound

**Files:** `include/FrameBuffer.h`, `src/FrameBuffer.cpp`

**Changes:**
1. Add `fillRect(int16_t x, int16_t y, uint8_t w, uint8_t h, Rgb color)` — fills a rectangle, X-coordinates wrap automatically via `setPixel`
2. Optimize: iterate columns with wrapped X, rows with bounds check
3. Rect that spans the cylinder boundary (x+w > 32) splits into two fill zones automatically due to X-wrapping

### Task 3: Add `drawLine` to `FrameBuffer` with wraparound

**Files:** `include/FrameBuffer.h`, `src/FrameBuffer.cpp`

**Changes:**
1. Add `drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, Rgb color)` — Bresenham's algorithm with X-wrapping
2. Each point goes through `setPixel(x, y)` which wraps X automatically
3. Y is bounds-checked (no vertical wrapping — this is a cylinder, not a torus)

### Task 4: Add `drawCircle` to `FrameBuffer`

**Files:** `include/FrameBuffer.h`, `src/FrameBuffer.cpp`

**Changes:**
1. Add `drawCircle(int16_t cx, int16_t cy, uint8_t r, Rgb color)` — midpoint circle algorithm
2. X wraps, Y is bounds-checked
3. Option: `fillCircle` variant

### Task 5: Verify all compiled effects render correctly

**Files:** `test/test_effects/test_main.cpp`

**Changes:**
1. Add test: `SolidColorEffect` fills 512 pixels with correct color
2. Add test: `AlternatingColumnsEffect` wraps correctly at boundary
3. Add test: `ClockOverlay` renders at all valid X positions
4. Test edge cases: effects that set pixels near x=0 and x=31

### Task 6: Add FrameBuffer wraparound tests

**Files:** `test/test_framebuffer/test_main.cpp`

**Changes:**
1. Add test: `fillRect` spanning the cylinder boundary: `fillRect(30, 0, 5, 16)` → columns 30,31,0,1,2 filled
2. Add test: `fillRect` at Y out of bounds — no effect
3. Add test: `drawLine` crossing x=0 boundary
4. Add test: `drawCircle` with center near boundary — pixels wrap correctly
5. Add test: `drawCircle` at edge — half visible

### Task 7: Add DSL rendering wraparound test

**Files:** `test/test_dsl_executor/test_main.cpp`

**Changes:**
1. Add test: sprite placed at x=31 renders wrapped (pixels at x=31,0,1,...)
2. Add test: animated x expression `x = t * 10` wraps correctly as it crosses 32

### Task 8: Update docs

**File:** `docs/ARCHITECTURE.md`

**Changes:** Document cylindrical coordinate system, wraparound semantics, angle-to-X mapping.

---

## 3. Verification Plan

### Unit Tests (native)
- `test_framebuffer` — all wraparound scenarios (fillRect, drawLine, drawCircle)
- `test_effects` — all 4 compiled effects on cylinder
- `test_dsl_executor` — sprite rendering at boundaries

### Integration Tests (on device)
1. **CYL-01**: `angleToX(0) == 0`, `angleToX(360) == 0` (wraps)
2. **CYL-02**: `setPixel(0, 0, red)` and `setPixel(32, 0, red)` write same physical LED
3. **CYL-03**: `fillRect(0, 0, 32, 16, white)` fills all 512 pixels without gaps
4. **CYL-04**: `fillRect(30, 0, 5, 2, red)` fills columns 30,31,0,1,2 of rows 0-1
5. **CYL-05**: All 4 effects render visually correctly (no artifacts at seam)
6. **CYL-06**: DSL effect with `x = 31` sprite appears at the cylinder seam

### Nyquist Validation
Each requirement must have ≥1 test:
- CYL-01: `angleToX()` unit test
- CYL-02: `setPixel` wraparound test
- CYL-03: `fillRect` full-fill test
- CYL-04: `fillRect` boundary-span test
- CYL-05: Each effect renders correctly
- CYL-06: DSL boundary sprite test

---

## 4. Risk Analysis

| Risk | Severity | Mitigation |
|------|----------|------------|
| fillRect spanning boundary is buggy | Medium | Thorough unit test for boundary case; pixel-by-pixel fallback via setPixel |
| Existing effects break due to coordinate changes | Low | X-wrapping already exists — no API change for setPixel |
| DSL effects render incorrectly at seam | Low | DSL uses setPixel which already wraps; test verifies this |
| drawLine Bresenham has edge cases at boundary | Medium | Use standard algorithm, each pixel goes through setPixel (auto-wraps) |
| Performance regression with new draw functions | Low | fillRect is simpler than looping setPixel; drawLine adds minimal overhead |
| fillCircle on small cylinder looks distorted | Low | Visual distortion is expected on cylinder — not a bug |

---

## 5. Task Execution Order

```
Task 1 (angle helpers) ──┐
                          ├──→ Task 2 (fillRect) ──→ Task 3 (drawLine) ──→ Task 4 (drawCircle)
                          │
                          └──→ Task 5 (effect tests) ── can be parallel with Tasks 2-4
                                Task 6 (framebuffer tests) ── after Tasks 2-4
                                Task 7 (DSL tests) ── after Tasks 2-4
                                Task 8 (docs) ── after all
```

Task 1 is a prerequisite for everything. Tasks 2-4 add features. Tasks 5-7 add test coverage. Task 8 is documentation.

---

## 6. UAT — Visual Validation on Device

**Run these DSL effects on the lamp to visually confirm cylindrical geometry.**

### Test A: Seam Wrap — Moving Vertical Line

```lux
effect "cyl_test_seam"

layer stripe {
  sprite rect_4x16 {
    bitmap """
    ####
    ####
    ####
    ####
    ####
    ####
    ####
    ####
    ####
    ####
    ####
    ####
    ####
    ####
    ####
    ####
    """
  }
  use rect_4x16
  color rgb(255, 60, 30)
  x = t * 4
  y = 0
  scale = 1
  visible = 1
}
```

**Expected:** Red vertical stripe smoothly moves right, disappears at x=32 and reappears at x=0. No jump, no gap. The seam is invisible.

### Test B: Full-Width Rectangle

```lux
effect "cyl_test_full_width"

sprite full_bar {
  bitmap """
  ################################
  ################################
  ################################
  ################################
  ################################
  ################################
  ################################
  ################################
  ################################
  ################################
  ################################
  ################################
  ################################
  ################################
  ################################
  ################################
  """
}

layer bar {
  use full_bar
  color hsv(t * 60, 1, 1)
  x = 0
  y = 0
  scale = 1
  visible = 1
}
```

**Expected:** Entire matrix filled with one solid colour. Colour cycles through rainbow. No dark seam at x=0/31 boundary. Matrix is uniformly lit.

### Test C: Dot at Seam

```lux
effect "cyl_test_seam_dot"

sprite dot {
  bitmap """
  #
  """
}

layer left_dot {
  use dot
  color rgb(255, 255, 255)
  x = 0
  y = 7
  scale = 2
  visible = 1
}

layer right_dot {
  use dot
  color rgb(255, 255, 255)
  x = 31
  y = 7
  scale = 2
  visible = 1
}
```

**Expected:** Two white dots at x=0 and x=31 — physically adjacent on the cylinder. They should appear NEXT to each other, not far apart.

### Test D: Circle Crossing Seam

```lux
effect "cyl_test_cross_seam"

sprite big_circle {
  bitmap """
      ########
    ############
   ##############
  ################
  ################
  ################
  ################
   ##############
    ############
      ########
  """
}

layer circle {
  use big_circle
  color rgb(80, 200, 255)
  x = t * 8
  y = 3
  scale = 1
  visible = 1
}
```

**Expected:** Blue circle moves right, crosses the seam smoothly. Circle should appear continuous when half is at x=30-31 and half at x=0-1.

### How to Run These Tests

1. Open the lamp web UI at `http://192.168.2.119`
2. Paste one test effect into the editor
3. Click **Запустить** (Run)
4. Observe the LED matrix
5. Repeat for each test

### Pass Criteria

| Test | What to check | CYL req |
|------|--------------|---------|
| A — Moving stripe | Smooth wraparound, no jump at seam | CYL-01, CYL-02 |
| B — Full-width bar | No dark column at x=0/31 | CYL-03, CYL-04 |
| C — Seam dots | Dots at x=0 and x=31 are neighbours | CYL-02 |
| D — Circle crossing | Circle intact when crossing seam | CYL-04, CYL-06 |
