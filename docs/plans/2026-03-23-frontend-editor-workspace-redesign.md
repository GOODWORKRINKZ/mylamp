# Frontend Editor Workspace Redesign Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Rebuild the embedded frontend into an editor-centered workspace with dropdown quick-access sections, header device modals, and a fixed bottom status bar.

**Architecture:** Extract the shell markup into a small pure renderer so the new layout can be tested without a browser, then wire the existing status, OTA, and network logic into the new DOM structure. Keep the handwritten DOM architecture, reuse the current network modal and OTA state handlers, and move runtime telemetry into a bottom status strip instead of the sidebar.

**Tech Stack:** TypeScript, Vite, existing DOM-based frontend, Node-based mock tests, PlatformIO embedded asset pipeline

---

### Task 1: Add a failing layout renderer test

**Files:**
- Create: `frontend/src/ui/shellTemplate.ts`
- Create: `frontend/test/layout.test.mjs`
- Modify: `frontend/package.json`

**Step 1: Write the failing test**

Create a runtime test that compiles a pure shell renderer and asserts the generated HTML includes:

- header round action buttons for `WiFi` and `FW`;
- dropdown workspace sections for presets, playlist, help, diagnostics;
- firmware modal container;
- fixed bottom status bar ids used by runtime status rendering.

**Step 2: Run test to verify it fails**

Run: `cd /home/builder/mylamp/frontend && npm test`
Expected: FAIL because the shell renderer module and/or expected layout markers do not exist yet.

**Step 3: Write the minimal implementation**

Create `frontend/src/ui/shellTemplate.ts` with a pure `renderShellMarkup()` function that accepts pre-rendered slot strings and returns the new page shell HTML.

**Step 4: Run test to verify it passes**

Run: `cd /home/builder/mylamp/frontend && npm test`
Expected: PASS for the layout renderer test and existing mock API test.

### Task 2: Rewire main frontend markup to the new shell

**Files:**
- Modify: `frontend/src/main.ts`
- Modify: `frontend/src/styles/app.css`

**Step 1: Write the failing build state**

Switch `frontend/src/main.ts` to consume the new shell renderer and reference the future DOM ids for header buttons, dropdown dock, firmware modal, and bottom status bar.

**Step 2: Run typecheck/build to verify it fails**

Run: `cd /home/builder/mylamp/frontend && npm exec tsc --noEmit`
Expected: FAIL because modal renderers, ids, or bindings are incomplete.

**Step 3: Write the minimal implementation**

Update `frontend/src/main.ts` to:

- use `renderShellMarkup()`;
- move OTA markup into a firmware modal renderer;
- bind header buttons for network and firmware modals;
- keep snippet/help rendering inside dropdown sections;
- keep action buttons and editor behavior intact.

**Step 4: Run typecheck to verify it passes**

Run: `cd /home/builder/mylamp/frontend && npm exec tsc --noEmit`
Expected: PASS.

### Task 3: Move runtime telemetry into fixed bottom status bar

**Files:**
- Modify: `frontend/src/main.ts`
- Modify: `frontend/src/styles/app.css`

**Step 1: Write the failing runtime mapping**

Point `renderStatus()` at the new bottom bar ids and remove dependency on the old runtime sidebar ids.

**Step 2: Run build to verify it fails or reveals missing ids**

Run: `cd /home/builder/mylamp/frontend && npm run build`
Expected: FAIL or surface missing DOM references caused by the layout migration.

**Step 3: Write the minimal implementation**

Add the bottom status strip markup and styles, update status text targets, and ensure the page adds bottom padding so the fixed bar does not cover content.

**Step 4: Run build to verify it passes**

Run: `cd /home/builder/mylamp/frontend && npm run build`
Expected: PASS.

### Task 4: Final verification of the embedded frontend

**Files:**
- Modify: `include/embedded_resources.h` (generated)

**Step 1: Run frontend tests**

Run: `cd /home/builder/mylamp/frontend && npm test`
Expected: PASS.

**Step 2: Run frontend typecheck**

Run: `cd /home/builder/mylamp/frontend && npm exec tsc --noEmit`
Expected: PASS.

**Step 3: Run frontend build**

Run: `cd /home/builder/mylamp/frontend && npm run build`
Expected: PASS.

**Step 4: Run firmware build**

Run: `cd /home/builder/mylamp && ~/.platformio/penv/bin/platformio run --environment esp32-c3-supermini-dev`
Expected: PASS and regenerated embedded assets.