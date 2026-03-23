# Live Coding Frontend and DSL Runtime Design

**Date:** 2026-03-23

## Goal

Build an editor-first web interface for mylamp that lets users write lamp effects, run them live on the device, save them as presets, and organize them into autoplay playlists. The device must stay autonomous after the browser disconnects.

## Product Constraints

- Target MCU is ESP32-C3 Super Mini.
- LED surface is a logical 32x16 cylindrical canvas.
- The UI must work in AP mode without external infrastructure.
- The device must keep running saved presets and playlists without a browser session.
- The system must stay small enough for OTA, Wi-Fi, web UI, time sync, and sensor polling to coexist.
- The UX should be friendly for Russian-speaking children.

## Architectural Decision Summary

### Frontend stack

- Use TypeScript.
- Use Vite to build static assets.
- Use CodeMirror 6 for the editor.
- Keep the frontend framework-light for v1. Do not introduce React or Vue unless the UI complexity proves it necessary later.

### Runtime model

- Effects are executed on the lamp, not in the browser.
- The browser sends DSL source to the device for validation, temporary execution, or saving as a preset.
- Manual live runs stop autoplay so the user has full control.

### Storage model

- Store presets and playlists in LittleFS as JSON documents.
- Store short operational settings in Preferences/NVS.
- Keep the currently active preset and autoplay state in NVS so the device can resume after reboot.

### Resource delivery

- Build the frontend into static assets.
- Embed built assets into the firmware using a microbbox-style pre-build script.
- Serve the embedded assets directly from flash in AP mode and station mode.

## System Shape

The feature is split into four layers.

### 1. Frontend

Responsibilities:

- DSL editor
- diagnostics panel
- preset manager
- playlist builder
- lamp settings UI
- runtime status and error display

Proposed structure:

```txt
frontend/
  index.html
  vite.config.ts
  package.json
  src/
    main.ts
    api/
    editor/
    presets/
    playlists/
    settings/
    status/
    styles/
```

### 2. Web API

The firmware exposes a small HTTP API. It remains thin and delegates real logic to runtime and storage services.

### 3. Live runtime

The firmware owns:

- DSL lexer/parser
- validator
- compiler to an internal program representation
- executor for frame rendering
- playlist scheduler

### 4. Storage

- LittleFS for content documents
- Preferences for short state

## DSL Design

### Language style

- Use an English-core DSL.
- Keep Russian UI labels, Russian diagnostics, Russian docs, Russian example names, and Russian autocomplete descriptions.
- Do not use Russian keywords in v1. Parser simplicity and future tooling matter more than localized syntax.

### DSL goals

- Small grammar
- Safe execution
- Learnable by children
- Easy to generate from templates and later from LLM prompts

### Primitive concepts

1. `effect`
2. `sprite`
3. `layer`
4. expression formulas
5. pixel color constructors

### Built-in variables and functions

Coordinates and time:

- `x`, `y`
- `nx`, `ny`
- `t`, `dt`

Sensors and environment:

- `temp()`
- `humidity()`

Math:

- `sin`, `cos`, `abs`, `min`, `max`, `clamp`, `mix`, `smoothstep`

Color:

- `rgb(r, g, b)`
- `hsv(h, s, v)`

### Sprite model

Users can define a custom set of pixels via bitmap masks.

Example:

```txt
sprite heart {
  bitmap """
  ..##..
  .####.
  ######
  ######
  .####.
  ..##..
  """
}
```

This is the primary v1 way to define shapes because it is visual and child-friendly.

### Layer model

Each layer references a sprite and applies transforms.

Supported v1 properties:

- `use`
- `x`
- `y`
- `scale`
- `color`
- `visible`

Layers are rendered in declaration order.

### Example effect

```txt
effect "flying_shapes"

sprite heart {
  bitmap """
  ..##..
  .####.
  ######
  ######
  .####.
  ..##..
  """
}

sprite lightning {
  bitmap """
  ..##..
  ..#...
  .##...
  ####..
  ..##..
  ..#...
  """
}

layer heart1 {
  use heart
  color rgb(255, 40, 80)
  x = 4 + t * 2
  y = 8 + sin(t * 1.5) * 3
  scale = 1.0 + sin(t * 3.0) * 0.25
}

layer bolt1 {
  use lightning
  color rgb(255, 220, 40)
  x = 27 - t * 2
  y = 8 + sin(t * 1.5 + 3.14) * 2
  scale = 1.1 + sin(t * 2.0) * 0.2
}
```

### Explicitly out of scope for v1

- arbitrary user-defined loops
- recursion
- user-defined functions
- general-purpose scripting
- sprite rotation
- blend modes
- collision systems

## Preset Model

Each preset is a first-class device object.

Example JSON:

```json
{
  "id": "warm_waves",
  "name": "Warm Waves",
  "source": "...dsl source...",
  "createdAt": "2026-03-23T18:30:00Z",
  "updatedAt": "2026-03-23T18:45:00Z",
  "tags": ["warm", "ambient"],
  "options": {
    "brightnessCap": 0.35
  }
}
```

Storage paths:

- `/presets/<id>.json`

Target scale for v1:

- 10 to 30 presets

## Playlist Model

Playlists are independent objects.

Example JSON:

```json
{
  "id": "evening",
  "name": "Evening Loop",
  "repeat": true,
  "entries": [
    { "presetId": "warm_waves", "durationSec": 90, "enabled": true },
    { "presetId": "soft_clock", "durationSec": 60, "enabled": true },
    { "presetId": "fire_band", "durationSec": 45, "enabled": true }
  ]
}
```

Storage paths:

- `/playlists/<id>.json`

Autoplay behavior:

- the scheduler advances by `durationSec`
- manual `Run` stops autoplay immediately
- the device can resume the chosen preset or playlist after reboot

## API Surface

### Existing API to keep

- `GET /api/status`
- `GET /api/settings/network`
- `POST /api/settings/network`

### New live coding API

- `POST /api/live/validate`
- `POST /api/live/run`
- `POST /api/live/save`

### Preset API

- `GET /api/presets`
- `GET /api/presets/:id`
- `PUT /api/presets/:id`
- `DELETE /api/presets/:id`
- `POST /api/presets/:id/activate`

### Playlist API

- `GET /api/playlists`
- `POST /api/playlists`
- `PUT /api/playlists/:id`
- `DELETE /api/playlists/:id`
- `POST /api/playlists/:id/start`
- `POST /api/playlists/stop`

### Future settings API

- `GET /api/settings/clock`
- `POST /api/settings/clock`
- `GET /api/settings/display`
- `POST /api/settings/display`

## Diagnostics Model

Diagnostics must be structured and editor-friendly.

Example error payload:

```json
{
  "ok": false,
  "errors": [
    {
      "line": 4,
      "column": 9,
      "message": "Неизвестная функция hum. Возможно, ты хотел humidity()"
    }
  ]
}
```

Two layers are required:

1. client-side request validation
2. device-side parse and compile diagnostics

## UX for Russian-speaking children

- Russian UI labels everywhere.
- Russian preset names and starter examples.
- Russian tooltips and autocomplete descriptions.
- Russian diagnostics with helpful wording.
- Start with a library of templates instead of an empty editor.

Recommended starter examples:

- Радуга
- Огонь
- Теплые волны
- Летающее сердечко
- Молния
- Часы

## Limits and Safety

Suggested v1 limits:

- max preset source size: 4-8 KB
- max playlist length: 32 entries
- max preset count: 30
- compile budget and runtime operation budget per frame
- parser rejects unsupported constructs explicitly

These limits are necessary to keep the ESP32-C3 stable while Wi-Fi, web serving, time sync, sensors, and OTA remain active.

## Delivery Pipeline

1. Build frontend with Vite.
2. Emit static assets to a resource staging directory.
3. Run a Python embed script before PlatformIO build.
4. Generate `include/embedded_resources.h` with embedded assets.
5. Serve those assets from firmware.

The embed script should support build-time placeholder substitution for:

- `__VERSION__`
- `__CHANNEL__`
- `__BOARD__`

## Rollout Order

1. Frontend shell plus embedded resource pipeline
2. LittleFS content storage for presets and playlists
3. CodeMirror editor and live validation endpoint
4. DSL parser and minimal executor
5. Save, load, activate preset flows
6. Playlist scheduler and autoplay status
7. Diagnostics and UX polish

## Final Decision

The approved v1 direction is:

- TypeScript + Vite + CodeMirror frontend
- embedded static assets in firmware
- English-core DSL with Russian UX
- on-device DSL execution
- preset and playlist persistence on the device
- autoplay by playlist with duration-based switching
- manual live run stops autoplay