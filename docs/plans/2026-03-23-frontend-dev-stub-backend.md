# Frontend Dev Stub Backend Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Add a local Vite-powered frontend development mode with a stub backend and switchable lamp scenarios.

**Architecture:** The frontend keeps using real `/api/...` requests. In Vite dev mode, a mock middleware responds with in-memory data stores and scenario-dependent runtime state. Production build behavior remains unchanged.

**Tech Stack:** TypeScript, Vite 2, Vite dev middleware, browser localStorage

---

### Task 1: Add dev script and mock data model

**Files:**
- Modify: `frontend/package.json`
- Create: `frontend/src/dev/mockTypes.ts`
- Create: `frontend/src/dev/mockScenarios.ts`

**Step 1: Add the failing dev command expectation**

Document a new `npm run dev` script entry in `frontend/package.json` but do not wire the backend yet.

**Step 2: Run the dev build contract check**

Run: `cd frontend && npm run build`
Expected: PASS, confirming the added script metadata does not break production build.

**Step 3: Add the minimal mock type layer**

Create shared types for:

- status payload
- preset payloads
- playlist payloads
- live diagnostics payloads
- scenario ids

**Step 4: Add scenario seed data**

Create in-memory scenario presets for:

- `happy-path`
- `autoplay`
- `dsl-error`
- `offline-ish`
- `sensor-missing`

**Step 5: Commit**

```bash
git add frontend/package.json frontend/src/dev/mockTypes.ts frontend/src/dev/mockScenarios.ts
git commit -m "Add frontend dev mock scenario model"
```

### Task 2: Add Vite mock middleware

**Files:**
- Modify: `frontend/vite.config.ts`
- Create: `frontend/src/dev/mockApi.ts`

**Step 1: Write the failing middleware contract**

Make `vite.config.ts` import a planned mock API plugin that does not exist yet.

**Step 2: Run the build to verify failure**

Run: `cd frontend && npm run build`
Expected: FAIL with missing `mockApi` module.

**Step 3: Implement the mock API layer**

Add a dev-only Vite plugin/middleware that serves:

- `/api/status`
- `/api/live/validate`
- `/api/live/run`
- `/api/presets...`
- `/api/playlists...`

with in-memory state.

**Step 4: Re-run the build**

Run: `cd frontend && npm run build`
Expected: PASS.

**Step 5: Commit**

```bash
git add frontend/vite.config.ts frontend/src/dev/mockApi.ts
git commit -m "Add Vite mock backend middleware"
```

### Task 3: Add frontend dev scenario controls

**Files:**
- Modify: `frontend/src/main.ts`
- Modify: `frontend/src/styles/app.css`

**Step 1: Create the failing integration point**

Add scenario selector markup and state hooks in `main.ts` that expect dev scenario data helpers not yet wired.

**Step 2: Run frontend build to verify failure**

Run: `cd frontend && npm run build`
Expected: FAIL on missing scenario helper usage or typing mismatch.

**Step 3: Implement the minimal dev control panel**

Add a dev-only UI block that:

- shows active scenario
- lets the user switch scenarios
- persists selection in `localStorage`
- resets mock state

**Step 4: Style the dev panel**

Add compact styles consistent with the current shell.

**Step 5: Re-run frontend build**

Run: `cd frontend && npm run build`
Expected: PASS.

**Step 6: Commit**

```bash
git add frontend/src/main.ts frontend/src/styles/app.css
git commit -m "Add frontend dev scenario controls"
```

### Task 4: Verify local frontend flow

**Files:**
- Modify: `README.md`

**Step 1: Document local frontend development**

Add a short README section describing:

- `cd frontend && npm run dev`
- available scenarios
- query parameter override

**Step 2: Run production verification**

Run: `cd frontend && npm run build`
Expected: PASS.

**Step 3: Run local dev server verification**

Run: `cd frontend && npm run dev -- --host 127.0.0.1 --port 4173`
Expected: dev server starts successfully with mock backend enabled.

**Step 4: Commit**

```bash
git add README.md
git commit -m "Document local frontend stub backend"
```