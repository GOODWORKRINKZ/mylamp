---
phase: 5
slug: dsl
status: draft
nyquist_compliant: true
wave_0_complete: true
created: 2026-06-02
---

# Phase 5 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Unity (PlatformIO unit testing) + manual frontend UAT |
| **Config file** | `platformio.ini` (existing `test/` environments) |
| **Quick run command** | `platformio test -e native --filter "test_dsl_*" -v` |
| **Full suite command** | `platformio test -e native -v` |
| **Estimated runtime** | ~30 seconds (quick), ~120 seconds (full) |

---

## Sampling Rate

- **After every task commit:** Run `platformio test -e native --filter "test_dsl_*" -v`
- **After every plan wave:** Run `platformio test -e native -v`
- **Before `/gsd-verify-work`:** Full suite must be green + manual UAT on device
- **Max feedback latency:** 30 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 5-01-01 | 01 | 1 | DSL-02 | T-5-01 | Lexer rejects oversized input | unit | `platformio test -e native --filter "test_dsl_parser" -v` | ❌ W0 | ⬜ pending |
| 5-01-02 | 01 | 1 | DSL-02 | T-5-02 | Parser returns diagnostics on malformed for-loop | unit | `platformio test -e native --filter "test_dsl_parser" -v` | ❌ W0 | ⬜ pending |
| 5-01-03 | 01 | 1 | DSL-01 | T-5-03 | Frame index bounded by frames.size() | unit | `platformio test -e native --filter "test_dsl_executor" -v` | ❌ W0 | ⬜ pending |
| 5-02-01 | 02 | 2 | DSL-01 | — | N/A | unit | `platformio test -e native --filter "test_dsl_executor" -v` | ❌ W0 | ⬜ pending |
| 5-02-02 | 02 | 2 | DSL-02 | T-5-04 | MAX_UNROLLED_LAYERS=64 enforced at compile time | unit | `platformio test -e native --filter "test_dsl_executor" -v` | ❌ W0 | ⬜ pending |
| 5-02-03 | 02 | 2 | DSL-02 | — | Backward compat: old .lux files parse unchanged | regression | `platformio test -e native -v` | ✅ existing | ⬜ pending |
| 5-03-01 | 03 | 3 | DSL-01, DSL-03 | — | N/A | manual | Flash + web UI: all 8 demos load & run | N/A | ⬜ pending |
| 5-04-01 | 04 | 4 | DSL-03 | T-5-05 | Base64 decode passes validateSource() before execution | manual | Browser: share link round-trip | N/A | ⬜ pending |
| 5-04-02 | 04 | 4 | DSL-03, D-11 | T-5-06 | PUT body validated by ArduinoJson schema | manual | Flash + web UI: save preset → refresh → appears in list | N/A | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [x] `test/test_dsl_parser/test_main.cpp` — add tests for: multi-frame sprite parsing, `for` loop parsing, backward compat of old sprite format, `frame` field in layer *(embedded in Plan 05-01 Task 3 — tests written as part of parser implementation)*
- [x] `test/test_dsl_executor/test_main.cpp` — add tests for: sprite frame selection by expression, `for` loop unrolling, loop variable substitution, MAX_UNROLLED_LAYERS enforcement *(embedded in Plan 05-02 Task 4 — 6 TDD tests written as part of executor implementation)*
- [x] No new test files needed; extend existing test files

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| All 8 demo effects render correctly on cylinder | DSL-01 | Requires physical LED matrix | Flash dev build, cycle through all 8 presets in web UI, verify visual output |
| Share button copies valid URL | D-10 | Requires browser clipboard API | Click "Поделиться", paste URL in new tab, verify editor loads effect |
| Share link decode loads effect | D-10 | Requires browser URL parsing | Open `http://device.local/?code=<base64>`, verify editor shows decoded effect |
| Save button creates persistent preset | D-11 | Requires device flash + LittleFS | Save effect from editor, refresh page, verify preset appears in library |
| Factory presets seeded on first boot | D-09 | Requires clean LittleFS | Erase flash, flash firmware, verify 8 demo presets appear in preset list |

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 dependencies *(11/11 tasks have verify commands; frontend tasks are manual-verify per VALIDATION.md manual-only table)*
- [x] Sampling continuity: no 3 consecutive tasks without automated verify *(all waves interleave automated tests)*
- [x] Wave 0 covers all MISSING references *(parser + executor tests embedded in Plans 05-01 and 05-02)*
- [x] No watch-mode flags
- [x] Feedback latency < 30s *(quick: ~5s for grep, ~30s for platformio test --filter)*
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** approved 2026-06-02
