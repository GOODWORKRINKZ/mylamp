# Frontend OTA UI Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Add a full OTA control panel to the existing embedded frontend so users can view update state, switch channels, check for updates, and trigger installation from the browser.

**Architecture:** Keep the current single-screen frontend and add a focused OTA client/store alongside the existing status polling. Use the firmware OTA endpoints directly and extend the mock backend with the same contract so the UI can be developed without hardware.

**Tech Stack:** TypeScript, Vite, existing handwritten DOM frontend, mock API dev server, firmware OTA REST endpoints

---

### Task 1: Define OTA frontend contract

**Files:**
- Modify: `frontend/src/dev/mockTypes.ts`
- Test: `frontend/src/main.ts`

**Step 1: Write the failing contract change**

Add the OTA payload types needed by the frontend so current code does not compile until OTA rendering code is added.

**Step 2: Run build to verify failure**

Run: `cd frontend && npm run build`
Expected: build fails because OTA fields/types are referenced but not fully implemented.

**Step 3: Add the minimal OTA type definitions**

Define payloads for current OTA snapshot and OTA settings.

**Step 4: Run build again**

Run: `cd frontend && npm run build`
Expected: either next missing OTA references fail, or build passes for this slice.

**Step 5: Commit**

```bash
git add frontend/src/dev/mockTypes.ts
git commit -m "Add OTA frontend payload types"
```

### Task 2: Extend the mock backend with OTA endpoints

**Files:**
- Modify: `frontend/mockApi.mjs`
- Modify: `frontend/src/dev/mockScenarios.ts`
- Test: `frontend/mockApi.mjs`

**Step 1: Write the failing mock contract**

Add OTA endpoint handling expected by the frontend, leaving implementation intentionally incomplete first.

**Step 2: Verify it fails in dev or build flow**

Run: `cd frontend && npm run build`
Expected: frontend or local manual fetch path still cannot resolve the OTA payload shape.

**Step 3: Implement minimal OTA mock state and handlers**

Add mock storage for channel, OTA state, available version, and error. Implement `GET /api/update/current`, `GET /api/update/settings`, `POST /api/update/settings`, `POST /api/update/check`, and `POST /api/update/install`.

**Step 4: Verify the mock contract**

Run: `cd frontend && npm run build`
Expected: build continues cleanly with OTA mock support in place.

**Step 5: Commit**

```bash
git add frontend/mockApi.mjs frontend/src/dev/mockScenarios.ts
git commit -m "Add OTA mock API endpoints"
```

### Task 3: Render the OTA panel and client logic

**Files:**
- Modify: `frontend/src/main.ts`
- Modify: `frontend/src/styles/app.css`
- Test: `frontend/src/main.ts`

**Step 1: Write the failing UI integration**

Reference the new OTA fields and DOM nodes in `main.ts` before implementing the full rendering logic so the build goes red.

**Step 2: Run build to verify failure**

Run: `cd frontend && npm run build`
Expected: build fails because OTA DOM bindings or handlers are incomplete.

**Step 3: Implement minimal OTA rendering and actions**

Add the OTA panel markup, fetch helpers, channel switch handler, check/install actions, local busy-state handling, and OTA-specific status text updates.

**Step 4: Add the minimal styling**

Style the OTA card so it matches the current visual language and works on mobile.

**Step 5: Re-run build**

Run: `cd frontend && npm run build`
Expected: PASS.

**Step 6: Commit**

```bash
git add frontend/src/main.ts frontend/src/styles/app.css
git commit -m "Add OTA control panel to frontend"
```

### Task 4: Run final verification

**Files:**
- Modify: `README.md` (only if API/UI docs need a short note)

**Step 1: Rebuild frontend from a clean state**

Run: `cd frontend && npm run build`
Expected: PASS.

**Step 2: Verify firmware-facing contract still compiles if needed**

Run: `~/.platformio/penv/bin/platformio run --environment esp32-c3-supermini-dev`
Expected: PASS if embedded assets or API contract changed.

**Step 3: Commit**

```bash
git add README.md frontend
git commit -m "Finish OTA frontend UI"
```