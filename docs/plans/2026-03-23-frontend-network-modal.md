# Frontend Network Modal Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Add a modal-based network settings UI that lets users open a dedicated dialog, edit Wi-Fi/AP settings, and save them through the existing firmware API.

**Architecture:** Keep the current single-screen frontend and introduce a lightweight network modal state alongside the existing OTA and status flows. Reuse the handwritten DOM approach and extend the mock API so the modal can be built and verified without hardware.

**Tech Stack:** TypeScript, Vite, existing DOM-based frontend, mock API dev server, firmware network settings REST endpoints

---

### Task 1: Add failing network settings mock integration test

**Files:**
- Modify: `frontend/test/mock-api.test.mjs`
- Test: `frontend/mockApi.mjs`

**Step 1: Write the failing test**

Extend the existing frontend mock API integration test to call `GET /api/settings/network` and `POST /api/settings/network`.

**Step 2: Run test to verify it fails**

Run: `cd /home/builder/mylamp/frontend && npm test`
Expected: FAIL because the mock API does not yet implement network settings endpoints.

**Step 3: Write the minimal mock API implementation**

Add seeded network settings state and handlers for `GET` and `POST` network settings routes.

**Step 4: Run test to verify it passes**

Run: `cd /home/builder/mylamp/frontend && npm test`
Expected: PASS for the network settings slice and existing OTA assertions.

**Step 5: Commit**

```bash
git add frontend/mockApi.mjs frontend/test/mock-api.test.mjs
git commit -m "Add frontend network settings mock API"
```

### Task 2: Add modal state types and modal UI wiring

**Files:**
- Modify: `frontend/src/dev/mockTypes.ts`
- Modify: `frontend/src/main.ts`
- Modify: `frontend/src/styles/app.css`

**Step 1: Write the failing UI reference**

Reference the network settings payload and modal DOM IDs in `frontend/src/main.ts` before implementing the full client logic.

**Step 2: Run build to verify it fails**

Run: `cd /home/builder/mylamp/frontend && npm run build`
Expected: FAIL because the network settings payload or modal bindings are incomplete.

**Step 3: Write the minimal implementation**

Add network settings types, modal markup, open/close logic, fetch/save handlers, and busy/error UI state.

**Step 4: Add modal styling**

Style the overlay, dialog, fields, and action row so the modal matches the existing visual language and remains mobile-safe.

**Step 5: Run build to verify it passes**

Run: `cd /home/builder/mylamp/frontend && npm run build`
Expected: PASS.

**Step 6: Commit**

```bash
git add frontend/src/dev/mockTypes.ts frontend/src/main.ts frontend/src/styles/app.css
git commit -m "Add frontend network settings modal"
```

### Task 3: Final verification of embedded frontend flow

**Files:**
- Modify: `include/embedded_resources.h` (generated)

**Step 1: Run frontend tests**

Run: `cd /home/builder/mylamp/frontend && npm test`
Expected: PASS.

**Step 2: Run frontend build**

Run: `cd /home/builder/mylamp/frontend && npm run build`
Expected: PASS.

**Step 3: Run firmware build**

Run: `cd /home/builder/mylamp && ~/.platformio/penv/bin/platformio run --environment esp32-c3-supermini-dev`
Expected: PASS and regenerated embedded assets.

**Step 4: Commit**

```bash
git add include/embedded_resources.h frontend
git commit -m "Finish frontend network settings modal"
```