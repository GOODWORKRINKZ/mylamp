# Live Coding Frontend and DSL Runtime Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Build an embedded TypeScript frontend, on-device DSL runtime, preset storage, and playlist autoplay system for mylamp.

**Architecture:** The frontend is built with Vite and embedded into firmware as static assets. The firmware exposes a small API, stores content in LittleFS and short state in Preferences, and runs a constrained DSL executor that renders effects directly on the device.

**Tech Stack:** PlatformIO, Arduino framework, TypeScript, Vite, CodeMirror 6, Python build scripts, Unity native tests, LittleFS, Preferences

---

### Task 1: Create frontend scaffold and build contract

**Files:**
- Create: `frontend/package.json`
- Create: `frontend/tsconfig.json`
- Create: `frontend/vite.config.ts`
- Create: `frontend/index.html`
- Create: `frontend/src/main.ts`
- Create: `frontend/src/styles/app.css`
- Modify: `README.md`

**Step 1: Add the failing resource integration test plan note**

Document in `README.md` that the current root handler is temporary and frontend assets will replace it.

**Step 2: Scaffold the minimal Vite app**

Add the frontend files with a shell app that renders placeholder sections for editor, diagnostics, presets, playlists, and settings.

**Step 3: Build the frontend locally**

Run: `cd frontend && npm install && npm run build`
Expected: Vite outputs a static bundle under the configured output directory.

**Step 4: Commit**

```bash
git add frontend README.md
git commit -m "Add frontend scaffold"
```

### Task 2: Add resource embedding pipeline

**Files:**
- Create: `scripts/embed_resources.py`
- Create: `include/embedded_resources.h` (generated)
- Modify: `platformio.ini`
- Modify: `include/BuildInfo.h`
- Modify: `src/web/LampWebServer.cpp`
- Test: `test/test_status_json/test_main.cpp`

**Step 1: Write the failing integration expectation**

Add a test or assertion point in firmware-facing code so root content is no longer a plain text placeholder.

**Step 2: Add the embed script**

Implement a microbbox-style script that reads built frontend assets, substitutes placeholders, and generates `include/embedded_resources.h`.

**Step 3: Hook the script into PlatformIO**

Register the script as a pre-build extra script in `platformio.ini`.

**Step 4: Serve embedded index and asset files**

Replace the plain text root response in `LampWebServer` with embedded asset responses.

**Step 5: Verify build**

Run: `~/.platformio/penv/bin/platformio run --environment esp32-c3-supermini-dev`
Expected: build succeeds and generated resources are included.

**Step 6: Commit**

```bash
git add scripts/embed_resources.py platformio.ini include/BuildInfo.h src/web/LampWebServer.cpp include/embedded_resources.h
git commit -m "Add embedded frontend resource pipeline"
```

### Task 3: Introduce LittleFS content storage

### Follow-up Task: Gzip embedded web resources

**Files:**
- Modify: `scripts/embed_resources.py`
- Modify: `include/embedded_resources.h` (generated)
- Modify: `include/web/LampWebServer.h`
- Modify: `src/web/LampWebServer.cpp`
- Test: `~/.platformio/penv/bin/platformio run --environment esp32-c3-supermini-dev`

**Step 1: Add the failing delivery expectation**

Document that embedded HTML, JS, and CSS should be stored in compressed form and served with the correct HTTP headers.

**Step 2: Compress assets during embedding**

Update `embed_resources.py` so it substitutes placeholders first, then gzip-compresses text assets before generating `include/embedded_resources.h`.

**Step 3: Serve gzip assets correctly**

Teach `LampWebServer` to send `Content-Encoding: gzip` and keep the original MIME type for embedded frontend resources.

**Step 4: Verify firmware build**

Run: `~/.platformio/penv/bin/platformio run --environment esp32-c3-supermini-dev`
Expected: build succeeds, embedded resources are regenerated, and firmware size does not grow unexpectedly.

**Step 5: Commit**

```bash
git add docs/plans/2026-03-23-live-coding-frontend-implementation.md scripts/embed_resources.py include/web/LampWebServer.h src/web/LampWebServer.cpp include/embedded_resources.h
git commit -m "Compress embedded web resources"
```

**Files:**
- Create: `include/storage/IFileStore.h`
- Create: `include/storage/ContentPaths.h`
- Create: `include/storage/LittleFsFileStore.h`
- Create: `src/storage/LittleFsFileStore.cpp`
- Create: `test/test_file_store/test_main.cpp`
- Modify: `platformio.ini`
- Modify: `src/main.cpp`

**Step 1: Write the failing native tests**

Add tests for path building and a fake file store contract for read/write/list/delete behavior.

**Step 2: Run the tests to verify failure**

Run: `~/.platformio/penv/bin/platformio test --environment native-test --filter test_file_store`
Expected: missing file store types or methods.

**Step 3: Implement the minimal abstractions**

Add the file store interfaces and the embedded LittleFS-backed implementation.

**Step 4: Initialize the file system in firmware**

Mount LittleFS during startup and fail gracefully with a clear status if mounting fails.

**Step 5: Run the focused tests**

Run: `~/.platformio/penv/bin/platformio test --environment native-test --filter test_file_store`
Expected: PASS.

**Step 6: Commit**

```bash
git add include/storage src/storage test/test_file_store platformio.ini src/main.cpp
git commit -m "Add content file storage layer"
```

### Task 4: Implement preset and playlist domain models

**Files:**
- Create: `include/live/PresetModel.h`
- Create: `include/live/PlaylistModel.h`
- Create: `include/live/PresetJson.h`
- Create: `include/live/PlaylistJson.h`
- Create: `src/live/PresetJson.cpp`
- Create: `src/live/PlaylistJson.cpp`
- Create: `test/test_preset_json/test_main.cpp`
- Create: `test/test_playlist_json/test_main.cpp`
- Modify: `platformio.ini`

**Step 1: Write failing JSON tests**

Add native tests for serialize/parse round trips of preset and playlist JSON.

**Step 2: Run them to verify failure**

Run: `~/.platformio/penv/bin/platformio test --environment native-test --filter test_preset_json --filter test_playlist_json`
Expected: missing model and JSON helpers.

**Step 3: Implement the minimal models and codecs**

Add exact fields from the approved design and reject malformed payloads.

**Step 4: Re-run the tests**

Run: same command as Step 2
Expected: PASS.

**Step 5: Commit**

```bash
git add include/live src/live test/test_preset_json test/test_playlist_json platformio.ini
git commit -m "Add preset and playlist models"
```

### Task 5: Add preset repository and playlist repository services

**Files:**
- Create: `include/live/PresetRepository.h`
- Create: `include/live/PlaylistRepository.h`
- Create: `src/live/PresetRepository.cpp`
- Create: `src/live/PlaylistRepository.cpp`
- Create: `test/test_preset_repository/test_main.cpp`
- Create: `test/test_playlist_repository/test_main.cpp`
- Modify: `platformio.ini`

**Step 1: Write failing repository tests**

Use fake file stores to test save, load, list, update, and delete behavior.

**Step 2: Run tests to verify failure**

Run: `~/.platformio/penv/bin/platformio test --environment native-test --filter test_preset_repository --filter test_playlist_repository`
Expected: repository types missing.

**Step 3: Implement minimal repositories**

Map IDs to file paths and use the JSON codecs from Task 4.

**Step 4: Run tests again**

Run: same command as Step 2
Expected: PASS.

**Step 5: Commit**

```bash
git add include/live src/live test/test_preset_repository test/test_playlist_repository platformio.ini
git commit -m "Add preset and playlist repositories"
```

### Task 6: Add live DSL diagnostics endpoint contract

**Files:**
- Create: `include/live/Diagnostic.h`
- Create: `include/live/LiveRequestJson.h`
- Create: `src/live/LiveRequestJson.cpp`
- Create: `test/test_live_request_json/test_main.cpp`
- Modify: `include/web/LampWebServer.h`
- Modify: `src/web/LampWebServer.cpp`
- Modify: `platformio.ini`

**Step 1: Write failing request/response tests**

Add tests for parsing live validation and run payloads and formatting diagnostic JSON.

**Step 2: Run them to verify failure**

Run: `~/.platformio/penv/bin/platformio test --environment native-test --filter test_live_request_json`
Expected: missing request/diagnostic helpers.

**Step 3: Implement the minimal JSON helpers**

Support source text input, preset naming, and structured diagnostics.

**Step 4: Add placeholder API routes**

Expose `POST /api/live/validate` and `POST /api/live/run` returning structured `not implemented` diagnostics for now.

**Step 5: Build firmware**

Run: `~/.platformio/penv/bin/platformio run --environment esp32-c3-supermini-dev`
Expected: PASS.

**Step 6: Commit**

```bash
git add include/live src/live include/web/LampWebServer.h src/web/LampWebServer.cpp test/test_live_request_json platformio.ini
git commit -m "Add live diagnostics API contract"
```

### Task 7: Implement DSL lexer and parser for v1 grammar

**Files:**
- Create: `include/live/dsl/Token.h`
- Create: `include/live/dsl/Lexer.h`
- Create: `include/live/dsl/Ast.h`
- Create: `include/live/dsl/Parser.h`
- Create: `src/live/dsl/Lexer.cpp`
- Create: `src/live/dsl/Parser.cpp`
- Create: `test/test_dsl_parser/test_main.cpp`
- Modify: `platformio.ini`

**Step 1: Write failing parser tests**

Cover:
- `effect` header
- `sprite` bitmap block
- `layer` declarations
- simple expressions for `x`, `y`, `scale`, `color`
- Russian diagnostic message text for parse errors

**Step 2: Run parser tests to verify failure**

Run: `~/.platformio/penv/bin/platformio test --environment native-test --filter test_dsl_parser`
Expected: missing lexer and parser.

**Step 3: Implement minimal lexer and parser**

Only support the approved v1 grammar. Reject anything else explicitly.

**Step 4: Re-run parser tests**

Run: same command as Step 2
Expected: PASS.

**Step 5: Commit**

```bash
git add include/live/dsl src/live/dsl test/test_dsl_parser platformio.ini
git commit -m "Add DSL lexer and parser"
```

### Task 8: Implement compiled program and executor

**Files:**
- Create: `include/live/runtime/CompiledProgram.h`
- Create: `include/live/runtime/Compiler.h`
- Create: `include/live/runtime/Executor.h`
- Create: `src/live/runtime/Compiler.cpp`
- Create: `src/live/runtime/Executor.cpp`
- Create: `test/test_dsl_executor/test_main.cpp`
- Modify: `platformio.ini`

**Step 1: Write failing executor tests**

Cover:
- sprite bitmap projection
- layer ordering
- animated `x`, `y`, `scale`
- sensor function stubs
- color emission to `FrameBuffer`

**Step 2: Run executor tests to verify failure**

Run: `~/.platformio/penv/bin/platformio test --environment native-test --filter test_dsl_executor`
Expected: compiler and executor missing.

**Step 3: Implement minimal compiler and executor**

Compile the parsed AST into a compact internal representation and render frames into the existing framebuffer.

**Step 4: Re-run executor tests**

Run: same command as Step 2
Expected: PASS.

**Step 5: Commit**

```bash
git add include/live/runtime src/live/runtime test/test_dsl_executor platformio.ini
git commit -m "Add DSL compiler and executor"
```

### Task 9: Integrate live runtime into main loop

**Files:**
- Modify: `src/main.cpp`
- Create: `include/live/runtime/RuntimeContext.h`
- Create: `include/live/runtime/LiveProgramService.h`
- Create: `src/live/runtime/LiveProgramService.cpp`
- Create: `test/test_live_program_service/test_main.cpp`
- Modify: `platformio.ini`

**Step 1: Write failing service tests**

Cover temporary run, activate preset, stop program, and autoplay stop-on-manual-run behavior.

**Step 2: Run focused tests to verify failure**

Run: `~/.platformio/penv/bin/platformio test --environment native-test --filter test_live_program_service`
Expected: missing live program service.

**Step 3: Implement the minimal service**

Bind the executor to the main render loop and expose current active program state.

**Step 4: Integrate into `main.cpp`**

Route effect rendering through the live runtime when an active program exists.

**Step 5: Re-run tests**

Run: same command as Step 2
Expected: PASS.

**Step 6: Commit**

```bash
git add include/live/runtime src/live/runtime src/main.cpp test/test_live_program_service platformio.ini
git commit -m "Integrate live program runtime"
```

### Task 10: Add preset API and activation flow

**Files:**
- Modify: `include/web/LampWebServer.h`
- Modify: `src/web/LampWebServer.cpp`
- Create: `test/test_preset_api/test_main.cpp`
- Modify: `platformio.ini`

**Step 1: Write failing preset API tests**

Cover list, get, save, update, delete, and activate behavior using pure helper functions where possible.

**Step 2: Run the tests to verify failure**

Run: `~/.platformio/penv/bin/platformio test --environment native-test --filter test_preset_api`
Expected: missing preset API helpers.

**Step 3: Implement the preset routes**

Wire repository and live runtime services into the web layer.

**Step 4: Build firmware**

Run: `~/.platformio/penv/bin/platformio run --environment esp32-c3-supermini-dev`
Expected: PASS.

**Step 5: Commit**

```bash
git add include/web/LampWebServer.h src/web/LampWebServer.cpp test/test_preset_api platformio.ini
git commit -m "Add preset management API"
```

### Task 11: Add playlist repository, scheduler, and API

**Files:**
- Create: `include/live/runtime/PlaylistScheduler.h`
- Create: `src/live/runtime/PlaylistScheduler.cpp`
- Create: `test/test_playlist_scheduler/test_main.cpp`
- Create: `test/test_playlist_api/test_main.cpp`
- Modify: `include/web/LampWebServer.h`
- Modify: `src/web/LampWebServer.cpp`
- Modify: `src/main.cpp`
- Modify: `platformio.ini`

**Step 1: Write failing scheduler tests**

Cover:
- start playlist
- advance by duration
- repeat behavior
- stop autoplay on manual run

**Step 2: Write failing playlist API tests**

Cover create, update, delete, start, and stop actions.

**Step 3: Run tests to verify failure**

Run: `~/.platformio/penv/bin/platformio test --environment native-test --filter test_playlist_scheduler --filter test_playlist_api`
Expected: missing scheduler and API helpers.

**Step 4: Implement scheduler and routes**

Advance the active preset based on elapsed time in the main loop.

**Step 5: Re-run focused tests**

Run: same command as Step 3
Expected: PASS.

**Step 6: Commit**

```bash
git add include/live/runtime src/live/runtime include/web/LampWebServer.h src/web/LampWebServer.cpp src/main.cpp test/test_playlist_scheduler test/test_playlist_api platformio.ini
git commit -m "Add playlist autoplay system"
```

### Task 12: Upgrade status payload and diagnostics UI hooks

**Files:**
- Modify: `include/web/StatusJsonBuilder.h`
- Modify: `src/web/StatusJsonBuilder.cpp`
- Modify: `test/test_status_json/test_main.cpp`
- Modify: `frontend/src/main.ts`
- Modify: `frontend/src/styles/app.css`

**Step 1: Write failing status JSON assertions**

Add fields for:
- active preset id/name
- autoplay enabled
- active playlist id
- live error summary

**Step 2: Run native tests to verify failure**

Run: `~/.platformio/penv/bin/platformio test --environment native-test --filter test_status_json`
Expected: missing fields in status JSON.

**Step 3: Implement the status extensions**

Populate them from the runtime services.

**Step 4: Update the frontend shell**

Display live runtime state, current preset, and autoplay state.

**Step 5: Re-run the focused test and frontend build**

Run: `~/.platformio/penv/bin/platformio test --environment native-test --filter test_status_json`

Run: `cd frontend && npm run build`

Expected: PASS.

**Step 6: Commit**

```bash
git add include/web/StatusJsonBuilder.h src/web/StatusJsonBuilder.cpp test/test_status_json/test_main.cpp frontend/src/main.ts frontend/src/styles/app.css
git commit -m "Expose live runtime status in UI"
```

### Task 13: Add Russian-friendly editor UX

**Files:**
- Create: `frontend/src/editor/snippets.ts`
- Create: `frontend/src/editor/help.ts`
- Modify: `frontend/src/main.ts`
- Modify: `frontend/src/styles/app.css`

**Step 1: Add the starter examples**

Create template entries for:
- Радуга
- Огонь
- Теплые волны
- Летающее сердечко
- Молния
- Часы

**Step 2: Add Russian hints and inline docs**

Expose human-readable descriptions of variables, functions, and directives.

**Step 3: Build frontend**

Run: `cd frontend && npm run build`
Expected: PASS.

**Step 4: Commit**

```bash
git add frontend/src/editor frontend/src/main.ts frontend/src/styles/app.css
git commit -m "Add Russian-friendly editor guidance"
```

### Task 14: Full verification and documentation pass

**Files:**
- Modify: `README.md`
- Modify: `docs/ARCHITECTURE.md`
- Modify: `docs/plans/2026-03-23-live-coding-frontend-design.md`

**Step 1: Update docs**

Add the frontend workflow, preset storage model, and playlist/autoplay behavior to project docs.

**Step 2: Run full native verification**

Run: `~/.platformio/penv/bin/platformio test --environment native-test`
Expected: all tests pass.

**Step 3: Run full embedded verification**

Run: `~/.platformio/penv/bin/platformio run --environment esp32-c3-supermini-dev`
Expected: build succeeds.

**Step 4: Run frontend verification**

Run: `cd frontend && npm run build`
Expected: build succeeds.

**Step 5: Commit**

```bash
git add README.md docs/ARCHITECTURE.md docs/plans/2026-03-23-live-coding-frontend-design.md
git commit -m "Document live coding architecture"
```