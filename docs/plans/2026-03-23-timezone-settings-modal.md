# Timezone Settings Modal Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Add persistent timezone settings with a dedicated time modal opened from the bottom status bar.

**Architecture:** Extend `AppSettings.clock` so timezone becomes runtime-configurable and persisted through the existing settings backend, then expose it through a new `/api/settings/time` API that the frontend modal can consume. Keep the existing handwritten DOM structure, reuse modal patterns from network/firmware flows, and make the status bar clock item the entry point.

**Tech Stack:** C++/Arduino firmware, PlatformIO native Unity tests, TypeScript, Vite, Node-based frontend mock tests

---

### Task 1: Add failing firmware tests for timezone settings model

**Files:**
- Modify: `test/test_settings_persistence/test_main.cpp`
- Modify: `test/test_time_runtime/test_main.cpp`
- Create: `test/test_time_settings_api/test_main.cpp`

**Step 1: Write the failing tests**

Add tests that assert:

- `AppSettingsPersistence` loads/saves `clock.timezone`;
- `TimeRuntimeService` uses `settings.clock.timezone` instead of hardcoded config value;
- time settings JSON helpers expose and validate timezone values.

**Step 2: Run tests to verify they fail**

Run: `cd /home/builder/mylamp && ~/.platformio/penv/bin/platformio test -e native-test -f test_settings_persistence -f test_time_runtime -f test_time_settings_api`
Expected: FAIL because timezone is not persisted or exposed yet.

**Step 3: Write the minimal implementation**

Add timezone to clock settings, persistence, time runtime service input, and time settings JSON helpers.

**Step 4: Run tests to verify they pass**

Run: `cd /home/builder/mylamp && ~/.platformio/penv/bin/platformio test -e native-test -f test_settings_persistence -f test_time_runtime -f test_time_settings_api`
Expected: PASS.

### Task 2: Add failing frontend tests for time settings flow

**Files:**
- Modify: `frontend/test/mock-api.test.mjs`
- Modify: `frontend/test/layout.test.mjs`
- Modify: `frontend/mockApi.mjs`
- Modify: `frontend/package.json`

**Step 1: Write the failing tests**

Extend frontend tests to assert:

- `GET/POST /api/settings/time` works in mock API;
- shell markup includes clickable clock action element and time modal container.

**Step 2: Run tests to verify they fail**

Run: `cd /home/builder/mylamp/frontend && npm test`
Expected: FAIL because time settings routes/modal markers do not exist yet.

**Step 3: Write the minimal implementation**

Add mock state for time settings and extend the shell template with a clickable clock status item plus time modal placeholder.

**Step 4: Run tests to verify they pass**

Run: `cd /home/builder/mylamp/frontend && npm test`
Expected: PASS.

### Task 3: Wire time modal into runtime frontend

**Files:**
- Modify: `frontend/src/dev/mockTypes.ts`
- Modify: `frontend/src/dev/mockScenarios.ts`
- Modify: `frontend/src/ui/shellTemplate.ts`
- Modify: `frontend/src/main.ts`
- Modify: `frontend/src/styles/app.css`

**Step 1: Write the failing compile state**

Reference new time settings payloads, modal controls, and clock action ids in `frontend/src/main.ts`.

**Step 2: Run typecheck to verify it fails**

Run: `cd /home/builder/mylamp/frontend && npm exec -- tsc --noEmit`
Expected: FAIL because the time modal wiring is incomplete.

**Step 3: Write the minimal implementation**

Implement:

- time settings modal renderer;
- open/load/save/close logic;
- clickable clock item in status bar;
- status refresh after save;
- cleanup of obsolete sidebar id writes left from the layout migration.

**Step 4: Run typecheck/build to verify it passes**

Run: `cd /home/builder/mylamp/frontend && npm exec -- tsc --noEmit`
Expected: PASS.

### Task 4: Final verification

**Files:**
- Modify: `include/embedded_resources.h` (generated)

**Step 1: Run firmware native tests**

Run: `cd /home/builder/mylamp && ~/.platformio/penv/bin/platformio test -e native-test -f test_settings_persistence -f test_time_runtime -f test_time_settings_api`
Expected: PASS.

**Step 2: Run frontend tests**

Run: `cd /home/builder/mylamp/frontend && npm test`
Expected: PASS.

**Step 3: Run frontend typecheck**

Run: `cd /home/builder/mylamp/frontend && npm exec -- tsc --noEmit`
Expected: PASS.

**Step 4: Run frontend build**

Run: `cd /home/builder/mylamp/frontend && npm run build`
Expected: PASS.

**Step 5: Run firmware build**

Run: `cd /home/builder/mylamp && ~/.platformio/penv/bin/platformio run --environment esp32-c3-supermini-dev`
Expected: PASS.