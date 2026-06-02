# Phase 5: Demo Effects & DSL — Research

**Researched:** 2026-06-02
**Domain:** DSL language extension, sprite animation, embedded web frontend
**Confidence:** HIGH

## Summary

Phase 5 extends the Lux DSL with two new constructs — multi-frame `sprite` declarations and `for` loops — to enable 7-8 demo effects that showcase the lamp's capabilities. The phase also fixes the preset save bug (D-11), adds a base64 share link (D-10), and pre-seeds factory presets on first boot (D-09).

The DSL pipeline (`Lexer → Parser → Compiler → Executor`) requires surgical extensions at every layer. The `for` loop is the most invasive change: it introduces an imperative construct into an otherwise declarative language. The implementation must balance expressiveness (enough for Snake, Scrolling Text, and Plasma) against ESP32-C3 memory constraints. The recommended approach keeps `for` loops at the top level, generating multiple `CompiledLayer` instances at compile time via loop unrolling — avoiding runtime loop evaluation entirely.

The preset save bug is traced to the ESP32 WebServer library's handling of `PUT` request bodies: `server_.arg("plain")` may not be populated for PUT. The fix is to switch the frontend to use `PUT /api/presets/<id>` (the path-based route already registered in `LampWebServer::handlePresetByPath()`) instead of `PUT /api/presets?id=<id>`, and ensure the body reading path is identical to the working POST route.

**Primary recommendation:** Implement `for` loops as compile-time unrolling (not runtime iteration), implement `sprite` frames as `std::vector<CompiledSprite>` with frame index selected by a compiled expression, and fix the save bug by switching to path-based `PUT /api/presets/<id>`.

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Sprite frame parsing (`frame` keyword) | Parser (C++) | Lexer (C++) | New token type + new sprite block member |
| Frame selection expression (`frame = expr`) | Compiler (C++) | Executor (C++) | Expression compiled to node tree; executor evaluates per-frame |
| `for` loop parsing | Parser (C++) | Lexer (C++) | New top-level construct; parsed into AST before compilation |
| `for` loop execution | Compiler (C++) | — | Compile-time unrolling — no runtime loop support needed |
| Per-pixel fullscreen effects (Plasma) | Executor (C++) | Compiler (C++) | Already supported via `nx`/`ny` in per-pixel `color` expressions |
| Preset save (D-11 fix) | Frontend (TS) | WebServer (C++) | Frontend switches route; server-side body parsing unchanged |
| Base64 share link | Frontend (TS) | — | Pure browser feature: encode + clipboard API |
| Factory preset seeding | main.cpp (C++) | PresetRepository (C++) | First-boot check in setup(); writes via existing `save()` |
| Syntax highlighting | Frontend (TS) | — | Add `for`/`frame` to keyword set in `luxHighlight.ts` |
| Editor help | Frontend (TS) | — | Add `for`/`frame` help items in `help.ts` |
| Starter snippets | Frontend (TS) | — | Add 7-8 demo entries to `snippets.ts` |

## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D-01:** Named frames in `sprite`. Syntax: `sprite name { frame walk1 { bitmap "..." } frame walk2 { bitmap "..." } }`. Each frame is a separate bitmap inside sprite.
- **D-02:** Frame selection via `frame = <expression>` in `layer` (integer expression). E.g. `frame = (t * 4) % 3` for 3-frame cycle.
- **D-03:** `for` loops: `for <var> = <start>; <var> < <end>; <var> = <var> + <step> { ... }`. Loop variable visible only inside body. Needed for fullscreen effects without manual 32×16 bitmaps.
- **D-04:** Backward compatibility — all existing `.lux` effects must work unchanged. Old `sprite` without `frame` = single default frame. Old `layer` without `frame` = frame 0.
- **D-05:** `layer.use` remains sprite name literal. Frame selection is via new `frame` field, not through `use`.
- **D-06:** Required demos: Nyan Cat, Mario, Plasma/Perlin, Scrolling Text, Snake.
- **D-07:** Additional 2-3 effects from pool: Fire/particles, Star field, DNA spiral, DVD logo.
- **D-08:** ALL demo effects written in Lux DSL (not C++ IEffect).
- **D-09:** Demos available as starter snippets AND as pre-seeded presets.
- **D-10:** Share button encodes DSL code as base64 URL.
- **D-11:** Fix preset save from editor (runTemporary works, save doesn't).

### the agent's Discretion
- Exact `for` syntax: comparison operators (`<` only, or also `<=`/`>`/`>=`), nested loops
- Frame ordering: index-based internally, names for user
- Base64 format: standard vs URL-safe, prefix or no prefix
- Which 2-3 additional effects from the pool
- Exact pixel art for sprites (Mario, Nyan Cat) — adapted for 32×16 cylinder

### Deferred Ideas (OUT OF SCOPE)
- Auto-save history
- Preview/thumbnails for effects
- Custom fonts (user-defined, not built-in 3×5) — built-in font sufficient for demos
- Arrays/data tables in DSL — not needed for current demos

## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| DSL-01 | Demo effects with sprite animation on cylinder (e.g. Mario running) | §Sprite Frames, §Demo Effects Design |
| DSL-02 | Lux language extended for sprites/pixel arrays if needed | §DSL Grammar Changes, §For Loop Implementation |
| DSL-03 | Demo effects available as presets in web interface | §Frontend Changes, §Factory Preset Seeding |

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Existing DSL pipeline (Lexer/Parser/Compiler/Executor) | — | Parsing and executing .lux effects | Already integrated; phase extends, doesn't replace |
| ArduinoJson 6.x | (existing) | JSON parsing for preset save/load | Already used in PresetApi, PresetJson |
| LittleFS | (ESP32 built-in) | Preset file storage | Already used via LittleFsFileStore |
| ESP32 WebServer | (ESP32 built-in) | HTTP API for save/load/share | Already wired in LampWebServer |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `navigator.clipboard.writeText()` | Browser API | Copy share link to clipboard | Frontend share button |
| `btoa()` / `atob()` | Browser API | Base64 encode/decode | Share link encoding |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Compile-time `for` unrolling | Runtime loop bytecode | Runtime loops require new bytecode ops + loop stack in Executor; compile-time unrolling uses zero additional RAM and reuses existing layer rendering |
| URL-safe base64 (RFC 4648 §5) | Standard base64 with `+/=` | Standard base64 needs URL-encoding; URL-safe variant (`-_` no padding) is cleaner for share links. **Recommendation: URL-safe base64.** |
| Path-based preset save (`PUT /api/presets/<id>`) | Query-param save (`PUT /api/presets?id=<id>`) | Path-based already works in `handlePresetByPath()`; query-param route is the broken one. Switch to path-based. |

**Installation:** No new packages required. All dependencies are in-tree or browser built-ins.

## Architecture Patterns

### System Architecture Diagram

```
┌──────────────────────────────────────────────────────────────────┐
│                        WEB BROWSER                                │
│  ┌──────────┐  ┌──────────┐  ┌───────────┐  ┌────────────────┐  │
│  │ Editor   │  │ Snippets │  │ Share     │  │ Preset Library │  │
│  │ (lux src)│  │ (picker) │  │ (base64)  │  │ (save/load)    │  │
│  └────┬─────┘  └────┬─────┘  └─────┬─────┘  └───────┬────────┘  │
│       │             │              │                 │           │
│       ▼             ▼              ▼                 ▼           │
│  ┌──────────────────────────────────────────────────────────┐    │
│  │                    fetch() API calls                      │    │
│  │  POST /api/live/run   PUT /api/presets/<id>   GET /api/presets │
│  └──────────────────────────┬───────────────────────────────┘    │
└─────────────────────────────┼────────────────────────────────────┘
                              │ HTTP (Wi-Fi)
┌─────────────────────────────┼────────────────────────────────────┐
│                   ESP32-C3 (LampWebServer)                        │
│                             ▼                                     │
│  ┌──────────────────────────────────────────────────────────┐    │
│  │                    Route Handlers                         │    │
│  │  LiveApi  │  PresetApi  │  PlaylistApi  │  StatusApi     │    │
│  └────┬──────┴──────┬──────┴───────┬───────┴───────┬────────┘    │
│       │             │              │               │             │
│       ▼             ▼              ▼               ▼             │
│  ┌────────────────────────────────────────────────────────┐      │
│  │              LiveProgramService                         │      │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────────────────┐  │      │
│  │  │ Lexer    │→ │ Parser   │→ │ Compiler             │  │      │
│  │  │ (tokens) │  │ (AST)    │  │ (AST→CompiledProgram) │  │      │
│  │  └──────────┘  └──────────┘  └──────────┬───────────┘  │      │
│  │                                         │               │      │
│  │  ┌──────────────────────────────────────┼───────────┐  │      │
│  │  │              Executor                ▼           │  │      │
│  │  │  evaluateNode() → per-pixel color + blend       │  │      │
│  │  │  renderSpritePixel() → FrameBuffer               │  │      │
│  │  └──────────────────────┬───────────────────────────┘  │      │
│  └─────────────────────────┼──────────────────────────────┘      │
│                            ▼                                     │
│  ┌──────────────────────────────────────────────────────────┐    │
│  │  PresetRepository (LittleFS)  ←  Factory presets seeded   │    │
│  │  /presets/<id>.json           ←  on first boot            │    │
│  └──────────────────────────────────────────────────────────┘    │
│                            ▼                                     │
│  ┌──────────────────────────────────────────────────────────┐    │
│  │  FrameBuffer → MatrixLayout → FastLED → WS2812B panels   │    │
│  └──────────────────────────────────────────────────────────┘    │
└──────────────────────────────────────────────────────────────────┘
```

### Recommended Project Structure (new/changed files only)

```
include/live/dsl/
├── Ast.h              # MODIFY: SpriteDeclaration gains frames; new ForLoopStatement
├── Token.h            # MODIFY: add kKeywordFor, kKeywordFrame
include/live/runtime/
├── CompiledProgram.h  # MODIFY: CompiledSprite gains frames vector; new kLoopVar op
├── Compiler.h         # MODIFY: for-loop unrolling in compile()
├── Executor.h         # MODIFY: frame index evaluation per layer
src/live/dsl/
├── Lexer.cpp          # MODIFY: tokenize `for`, `frame` keywords; sprite block with frames
├── Parser.cpp         # MODIFY: parseFrameBlock(), parseForLoop()
src/live/runtime/
├── Compiler.cpp       # MODIFY: compileSpriteFrames(), unrollForLoops()
├── Executor.cpp       # MODIFY: select sprite frame by expression; evaluate loop var
src/
├── main.cpp           # MODIFY: seed factory presets on first boot
frontend/src/editor/
├── snippets.ts        # MODIFY: add 7-8 demo snippet entries
├── help.ts            # MODIFY: add `for`, `frame` help items
├── luxHighlight.ts    # MODIFY: add `for`, `frame` to keyword set
frontend/src/
├── main.ts            # MODIFY: fix save route; add share button handler
```

### Pattern 1: DSL Pipeline Extension (Lexer→Parser→Compiler→Executor)

**What:** Each DSL feature requires coordinated changes across all 4 pipeline stages. The pipeline is linear: Lexer tokenizes → Parser builds AST → Compiler produces CompiledProgram → Executor renders.

**When to use:** For every new keyword or syntax construct.

**Example — adding `frame` to sprite:**
```
Lexer:   TokenType::kKeywordFrame  ← tokenize "frame" inside sprite block
Parser:  SpriteDeclaration.frames[] ← parseFrameBlock() collects frame{name,bitmap}
Compiler: CompiledSprite.frames[]  ← compileSpriteFrames() processes each frame bitmap
Executor: select frame by index    ← evaluateNode(frameExpression) % frames.size()
```

### Pattern 2: Compile-Time Loop Unrolling

**What:** `for` loops are expanded at compile time into multiple `CompiledLayer` instances. The loop variable becomes a compile-time constant in each unrolled iteration.

**When to use:** For `for` loops in DSL. Avoids runtime loop VM entirely.

**Example:**
```
DSL:  for i = 0; i < 3; i = i + 1 { layer s{i} { use dot; x = i * 5; ... } }
After unrolling:
      layer s0 { use dot; x = 0; ... }
      layer s1 { use dot; x = 5; ... }
      layer s2 { use dot; x = 10; ... }
```

**Constraints:**
- Loop bounds must be compile-time constant (integer literals only)
- This is sufficient for ALL planned demo effects (Snake: ~10 segments, Scrolling Text: message length)
- Nested loops: allowed but each nesting multiplies layer count (risk of memory exhaustion)

### Anti-Patterns to Avoid
- **Runtime loop evaluation in Executor:** Would require new bytecode ops, loop stack, branch instructions, and break condition evaluation — massive complexity for ESP32-C3. Compile-time unrolling is simpler and sufficient.
- **`for` inside `layer` body:** The `for` loop belongs at the top level (alongside `sprite`/`layer`), generating multiple layers. Putting `for` inside `layer` would require per-pixel loop evaluation, which is both complex and unnecessary for planned demos.
- **Mixing `frame` with `use`:** Per D-05, `layer.use` stays sprite name only. `frame` is a separate field on `layer`. Don't support `use sprite.frame1` syntax.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Pixel art for sprites | Manual pixel-by-pixel drawing | Reference images + grid paper; then encode as `#`/`.` bitmap in DSL | Bitmap format is already the Lux standard; no need for new tools |
| Loop execution at runtime | Custom bytecode loop VM | Compile-time unrolling | Avoids VM complexity; all demo loops have small, constant bounds |
| Share link encoding | Custom encoding scheme | URL-safe base64 (`btoa`/`atob` with `+/=`→`-_` replacement) | Standard, debugable, no library needed |
| Preset seeding detection | Custom boot flag in NVS | Check if `/presets/` directory is empty via `PresetRepository::list()` | Reuses existing API; no new storage mechanism |
| Frame animation timing | Custom animation system | Reuse existing `t` variable in `frame = (t * fps) % frameCount` expression | DSL expression system already supports this |

**Key insight:** The existing DSL pipeline is well-architected for extension. Each layer (Lexer, Parser, Compiler, Executor) has a clear single responsibility, and new features fit into the existing patterns without architectural refactoring.

## DSL Grammar Changes

### Token.h — New Token Types
```cpp
// Add to enum class TokenType:
kKeywordFor,    // 'for' keyword
kKeywordFrame,  // 'frame' keyword inside sprite block
kLoopVariable,  // loop variable name (identifier in for-header)
```

### Ast.h — New/Modified Structures

**SpriteDeclaration (MODIFIED):**
```cpp
struct SpriteFrameDeclaration {
  std::string name;    // e.g. "walk1"
  std::string bitmap;  // same format as current sprite bitmap
};

struct SpriteDeclaration {
  std::string name;
  std::string bitmap;                          // OLD: single bitmap (backward compat)
  std::vector<SpriteFrameDeclaration> frames;  // NEW: multi-frame sprites
  // Invariant: if frames is non-empty, bitmap is ignored.
  // If frames is empty, bitmap is used (single-frame default).
};
```

**LayerDeclaration (MODIFIED):**
```cpp
struct LayerDeclaration {
  // ... existing fields ...
  std::string frameExpression;  // NEW: e.g. "(t * 4) % 3"
  uint32_t frameLine = 0;       // NEW: source line for diagnostics
};
```

**ForLoopStatement (NEW):**
```cpp
struct ForLoopStatement {
  std::string loopVariable;     // e.g. "i"
  std::string startExpression;  // e.g. "0"
  std::string endExpression;    // e.g. "snake_length" (but must be constant)
  std::string stepExpression;   // e.g. "1"
  std::vector<LayerDeclaration> body;  // layers generated by the loop
};
```

**Program (MODIFIED):**
```cpp
struct Program {
  std::string effectName;
  std::vector<SpriteDeclaration> sprites;
  std::vector<TextDeclaration> texts;
  std::vector<LayerDeclaration> layers;
  std::vector<ForLoopStatement> forLoops;  // NEW
};
```

### CompiledProgram.h — Modified Structures

**CompiledSprite (MODIFIED):**
```cpp
struct CompiledSprite {
  std::string name;
  int16_t width = 0;
  int16_t height = 0;
  std::vector<CompiledSpritePixel> pixels;       // OLD: single frame pixels
  std::vector<std::vector<CompiledSpritePixel>> frames;  // NEW: per-frame pixels
  // Invariant: if frames is non-empty, pixels is frame 0.
};
```

**CompiledLayer (MODIFIED):**
```cpp
struct CompiledLayer {
  // ... existing fields ...
  int16_t frameExpression = -1;  // NEW: expression index for frame selection; -1 = frame 0
};
```

**ExpressionOp (MODIFIED):**
```cpp
enum class ExpressionOp {
  // ... existing ops ...
  kLoopIndex,  // NEW: compile-time loop variable (replaced by constant during unrolling)
};
```

### Lexer Changes (`src/live/dsl/Lexer.cpp`)

**New keywords to tokenize:**
- `for` → `TokenType::kKeywordFor`
- `frame` (inside sprite block) → `TokenType::kKeywordFrame`

**Sprite block parsing changes:**
- After `sprite name {`, scan for either `bitmap """..."""` (old style) or `frame framename { bitmap """...""" }` (new style).
- Multiple `frame` blocks allowed inside one `sprite`.
- Closing `}` ends the sprite declaration.

**For loop parsing:**
- `for <identifier> = <expression>; <identifier> < <expression>; <identifier> = <identifier> + <expression> {`
- The body between `{` and `}` is tokenized as a nested block of layer declarations.
- The `;` and `<` / `+` / `=` in the for-header are tokenized as operators within the expression strings, not as separate tokens.

**Backward compatibility:** The lexer must detect which style a sprite uses. If the first token after `{` is `bitmap`, parse old style. If it's `frame`, parse new multi-frame style.

### Parser Changes (`src/live/dsl/Parser.cpp`)

**`parseSprite()` modification:**
```
After matching `{`:
  if next token is kKeywordBitmap → parse old-style (single bitmap)
  if next token is kKeywordFrame → parse new-style (loop: parse frame name, {, bitmap block, } until `}`)
Store in SpriteDeclaration.frames if multi-frame, else SpriteDeclaration.bitmap.
```

**`parseLayer()` modification:**
```
Add case for kKeywordFrame:
  match(kKeywordFrame)
  match(kEquals)
  Token expr = expect(kExpression)
  layer.frameExpression = expr.text
  layer.frameLine = expr.line
```

**New `parseForLoop()`:**
```
match(kKeywordFor)
match(kIdentifier) → loopVariable
match(kEquals)
match(kExpression) → startExpression
match(kSemicolon)  // or just expect it in expression
match(kIdentifier) → must match loopVariable
match(kLessThan)   // or kLessEqual, kGreaterThan, kGreaterEqual
match(kExpression) → endExpression
match(kSemicolon)
match(kIdentifier) → must match loopVariable
match(kEquals)
match(kIdentifier) → must match loopVariable
match(kPlus)
match(kExpression) → stepExpression
match(kLeftBrace)
// Parse body as sequence of layer declarations (reuse parseLayer)
while not kRightBrace: parseLayer() → add to forLoop.body
match(kRightBrace)
```

**Loop bound validation:** After parsing, validate that `startExpression`, `endExpression`, and `stepExpression` are compile-time constant (integer literals only). If not, emit diagnostic: "Границы цикла for должны быть целыми числами".

### Compiler Changes (`src/live/runtime/Compiler.cpp`)

**`compileSprite()` modification:**
- If `SpriteDeclaration.frames` is non-empty, compile each frame's bitmap into a `CompiledSpritePixel` vector, store in `CompiledSprite.frames`.
- Set `CompiledSprite.pixels` to `frames[0]` for backward compat (layer without `frame` renders frame 0).

**`compile()` modification — for-loop unrolling:**
```cpp
// After compiling sprites/texts and before compiling layers:
for (const ForLoopStatement& forLoop : program.forLoops) {
    int startVal = std::stoi(forLoop.startExpression);
    int endVal = std::stoi(forLoop.endExpression);
    int stepVal = std::stoi(forLoop.stepExpression);
    
    for (int i = startVal; i < endVal; i += stepVal) {
        for (LayerDeclaration layerTemplate : forLoop.body) {
            // Substitute loop variable in all layer expressions
            std::string iStr = std::to_string(i);
            layerTemplate.name += std::to_string(i);  // uniquify layer name
            replaceAll(layerTemplate.xExpression, forLoop.loopVariable, iStr);
            replaceAll(layerTemplate.yExpression, forLoop.loopVariable, iStr);
            replaceAll(layerTemplate.colorExpression, forLoop.loopVariable, iStr);
            replaceAll(layerTemplate.scaleExpression, forLoop.loopVariable, iStr);
            replaceAll(layerTemplate.rotationExpression, forLoop.loopVariable, iStr);
            replaceAll(layerTemplate.visibleExpression, forLoop.loopVariable, iStr);
            replaceAll(layerTemplate.frameExpression, forLoop.loopVariable, iStr);
            
            // Compile the generated layer normally
            compileLayer(layerTemplate, compiled);
        }
    }
}
```

**`compileLayer()` modification — frame expression:**
- If `layer.frameExpression` is non-empty, compile it as an expression via `ExpressionCompiler`.
- Store the resulting expression node index in `CompiledLayer.frameExpression`.

### Executor Changes (`src/live/runtime/Executor.cpp`)

**Frame selection in `render()`:**
```cpp
for (const CompiledLayer& layer : program.layers) {
    // ... existing spriteIndex check ...
    
    const CompiledSprite& sprite = program.sprites[layer.spriteIndex];
    
    // Determine which frame to render
    const std::vector<CompiledSpritePixel>* framePixels = &sprite.pixels;  // default: frame 0
    if (!sprite.frames.empty() && layer.frameExpression >= 0) {
        float frameFloat = evaluateNode(program.expressions, layer.frameExpression, baseContext);
        int frameIndex = static_cast<int>(std::fmod(std::abs(frameFloat), 
                                        static_cast<float>(sprite.frames.size())));
        framePixels = &sprite.frames[static_cast<size_t>(frameIndex)];
    }
    
    // Use framePixels instead of sprite.pixels for rendering
    for (const CompiledSpritePixel& pixel : *framePixels) {
        // ... existing pixel rendering logic ...
    }
}
```

## Preset Save Bug Analysis (D-11)

### Root Cause Investigation

**Symptom:** `POST /api/live/run` works (validates + runs source), but `PUT /api/presets?id=<id>` fails to save.

**Code path comparison:**

| Aspect | Live Run (working) | Preset Save (broken) |
|--------|-------------------|---------------------|
| HTTP method | POST | PUT |
| Route | `/api/live/run` | `/api/presets?id=<id>` |
| Body reading | `server_.arg("plain")` | `server_.arg("plain")` |
| Registration | `server_.on("/api/live/run", HTTP_POST, ...)` | `server_.on("/api/presets", HTTP_PUT, ...)` |

**Primary hypothesis:** The ESP32 WebServer library (ESP8266WebServer/WebServer) populates `server_.arg("plain")` for POST requests but does NOT consistently populate it for PUT requests. This is a known limitation in some versions of the library — the raw body is only buffered for POST by default. [ASSUMED — based on ESP32 WebServer library behavior; needs device-side verification]

**Secondary hypothesis:** There is ALSO a working PUT route: `PUT /api/presets/<id>` is handled by `LampWebServer::handlePresetByPath()`, which also calls `handlePutPresetRequest()` with `server_.arg("plain")`. If this path-based route works while the query-param route doesn't, the issue is specifically with query parameter handling in PUT requests.

**Recommended fix:** Switch the frontend from query-param PUT to path-based PUT:

```typescript
// OLD (broken):
const response = await fetch(`/api/presets?id=${encodeURIComponent(presetId)}`, {
    method: "PUT", ...
});

// NEW (fix):
const response = await fetch(`/api/presets/${encodeURIComponent(presetId)}`, {
    method: "PUT", ...
});
```

The server already registers `handlePresetByPath()` for `/api/presets/*` which handles PUT correctly. No server-side changes needed.

**Fallback if path-based PUT also fails:** Fall back to POST for save operations. Add a new route `POST /api/presets/<id>` that calls `handlePutPresetRequest()`. POST body reading is confirmed working.

## Base64 Share Link Implementation (D-10)

### Approach

**Encoding (frontend, on share button click):**
```typescript
function shareEffect(): void {
    const source = getEditorValue().trim();
    if (!source) return;
    
    // Encode to URL-safe base64
    const base64 = btoa(unescape(encodeURIComponent(source)));
    const urlSafe = base64.replace(/\+/g, '-').replace(/\//g, '_').replace(/=+$/, '');
    
    // Build share link
    const shareUrl = `${window.location.origin}?code=${urlSafe}`;
    
    // Copy to clipboard
    navigator.clipboard.writeText(shareUrl).then(() => {
        setText("editor-status", "Ссылка скопирована. Отправь её кому угодно.");
    });
}
```

**Decoding (frontend, on page load if `?code=` present):**
```typescript
function loadFromShareLink(): void {
    const params = new URLSearchParams(window.location.search);
    const encoded = params.get("code");
    if (!encoded) return;
    
    // Decode from URL-safe base64
    let base64 = encoded.replace(/-/g, '+').replace(/_/g, '/');
    while (base64.length % 4) base64 += '=';
    
    const source = decodeURIComponent(escape(atob(base64)));
    setEditorValue(source);
    
    // Clean URL
    window.history.replaceState({}, "", window.location.pathname);
}
```

**Format decision:** URL-safe base64 (RFC 4648 §5) without padding. This produces clean, copy-pasteable URLs like `http://mylamp.local/?code=ZWZmZWN0ICJueWFu...` that survive messaging apps without breaking.

**Button placement:** Add a "Поделиться" button next to the existing "Сохранить" button in the editor toolbar.

## Factory Preset Seeding (D-09)

### Implementation

**Trigger:** First boot detection. Check if `/presets/` directory is empty via `PresetRepository::list().empty()` after LittleFS is mounted.

**Location:** `src/main.cpp`, in `setup()`, after LittleFS initialization and `PresetRepository` construction, before `LampWebServer::begin()`.

**Code sketch:**
```cpp
// In setup(), after g_fileStore and g_presetRepository are initialized:
if (g_presetRepository.isReady() && g_presetRepository.list().empty()) {
    // Seed factory presets
    static constexpr const char* kFactoryPresets[] = {
        // Each entry: "id\0name\0source\0" (double-null terminated or separate arrays)
        // Use PresetModel with preset.id, preset.name, preset.source
    };
    
    for (const auto& source : kFactoryDemoSources) {
        lamp::live::PresetModel preset;
        preset.id = buildPresetId(source);  // extract from effect "name"
        preset.name = extractEffectName(source);
        preset.source = source;
        preset.createdAt = "2026-01-01T00:00:00Z";  // factory date
        preset.updatedAt = preset.createdAt;
        g_presetRepository.save(preset);
    }
}
```

**Design decision:** Factory presets are hardcoded as C++ string literals in `main.cpp` (or a separate `factory_presets.h`). This avoids needing to bundle `.lux` files as separate assets. The DSL source strings are compiled into the firmware binary.

**Preset IDs:** Use filesystem-safe IDs derived from effect names (e.g., `nyan-cat`, `mario-run`, `plasma`, `scrolling-text`, `snake`).

## Demo Effects Design

### Required Effects (D-06)

| Effect | DSL Features Used | Complexity | Notes |
|--------|-------------------|------------|-------|
| **Nyan Cat** | Multi-frame sprite, `frame` expression, per-pixel rainbow trail | MEDIUM | Cat sprite: 4-6 frames of running animation. Rainbow trail: separate fullscreen layer with per-pixel `hsv()` based on distance from cat position. |
| **Mario** | Multi-frame sprite, `frame` expression | LOW | 2-4 walk frames. Mario runs around cylinder circumference (x animated with sin/cos). Classic 8-bit color palette. |
| **Plasma/Perlin** | Per-pixel `color` expression, `sin`/`cos`/`nx`/`ny` | LOW | No sprites needed. Pure fullscreen per-pixel math. Similar to existing "upside-down" effect but with brighter, more colorful palette. |
| **Scrolling Text** | `text` sprite, `for` loop for character positioning | MEDIUM | Text scrolls around cylinder. `for` loop generates one layer per character, positioned at `x = offset + i * charWidth`. |
| **Snake** | `for` loop for segment drawing | MEDIUM | Snake body drawn as N segments using `for` loop. Head position animated with `sin`/`cos`. Body follows head with offset. |

### Additional Effects (D-07) — Recommended Selection

| Effect | DSL Features Used | Rationale |
|--------|-------------------|-----------|
| **Fire/Particles** | Per-pixel `color`, `sin`, `random`-like pattern | Visually striking, shows dynamic per-pixel effects. Different from Plasma (warm palette, upward particle motion). |
| **Star Field** | `for` loop for stars, per-pixel twinkle | Shows `for` loop utility for many small objects. Parallax scrolling effect. |
| **DNA Spiral** | `for` loop, `sin`/`cos` positioning | Visually unique on cylinder (double helix wraps around). Demonstrates mathematical pattern generation. |

**Total:** 8 demo effects (5 required + 3 additional). All written in Lux DSL.

### Pixel Art Design Notes

**Canvas:** 32×16 logical pixels. But note the XY-swap in `Executor::renderSpritePixel()`: DSL `x` = physical Y (cylinder circumference, 32, wraps), DSL `y` = physical X (cylinder height, 16, bounded).

**Nyan Cat sprite:** Body ~8×8 pixels (pop-tart shape), tail ~4 pixels. Must fit within 16px height constraint. Reference pixel art provided in CONTEXT.md.

**Mario sprite:** 8×8 pixels typical for 8-bit Mario. 2-4 walk frames. Must fit within 16px height.

**Scrolling Text:** Built-in 3×5 font. Each character is 3px wide + 1px spacing = 4px. On 32px circumference, ~8 characters visible at once. For scrolling, the `for` loop generates a layer per character.

## Frontend Changes

### snippets.ts — New Starter Snippets

Add 7-8 entries to the `starterSnippets` array:
```typescript
{ id: "nyan-cat", name: "Nyan Cat", description: "Котик с радужным шлейфом.", source: `...` },
{ id: "mario-run", name: "Марио", description: "8-битный Марио бежит по кругу.", source: `...` },
{ id: "plasma", name: "Плазма", description: "Абстрактные переливы на всю матрицу.", source: `...` },
{ id: "scrolling-text", name: "Бегущая строка", description: "Текст движется по окружности.", source: `...` },
{ id: "snake", name: "Змейка", description: "Классическая змейка на цилиндре.", source: `...` },
{ id: "fire-particles", name: "Огонь", description: "Частицы пламени поднимаются вверх.", source: `...` },
{ id: "starfield", name: "Звёзды", description: "Звёздное поле с параллаксом.", source: `...` },
{ id: "dna-spiral", name: "ДНК", description: "Двойная спираль на цилиндре.", source: `...` },
```

### help.ts — New Help Items

Add to `editorHelpSections`:
```typescript
{
  title: "Анимация",
  items: [
    { term: "frame name { bitmap ... }", description: "Кадр анимации внутри sprite. Можно объявить несколько frame-ов." },
    { term: "frame = выражение", description: "Выбор кадра в layer. Выражение округляется до целого. Например: frame = (t * 4) % 3" },
  ],
},
{
  title: "Циклы",
  items: [
    { term: "for i = 0; i < N; i = i + 1 { ... }", description: "Цикл, который повторяет layer-ы N раз. Переменная i подставляется в выражения." },
  ],
},
```

### luxHighlight.ts — New Keywords

Add to `LUX_KEYWORDS`:
```typescript
const LUX_KEYWORDS = new Set([
  "effect", "sprite", "text", "layer", "use", "color", "bitmap",
  "visible", "x", "y", "scale", "rotation", "blend",
  "for", "frame",  // NEW
]);
```

### main.ts — Fixes and Additions

**1. Save route fix (D-11):**
```typescript
// OLD: const response = await fetch(`/api/presets?id=${encodeURIComponent(presetId)}`, {
// NEW:
const response = await fetch(`/api/presets/${encodeURIComponent(presetId)}`, {
    method: "PUT",
    headers: buildJsonHeaders(),
    body: JSON.stringify({...}),
});
```

Also update the response handling — the path-based route returns the full preset JSON, same as the query-param route.

**2. Share button (D-10):**
Add button in editor toolbar (in `shellTemplate.ts` or directly in `main.ts`):
```html
<button id="share-button" type="button">Поделиться</button>
```

Bind in `bindActionButtons()`:
```typescript
const shareButton = document.getElementById("share-button") as HTMLButtonElement | null;
shareButton?.addEventListener("click", () => { shareEffect(); });
```

**3. Share link decoding on load:**
Add to the initialization code:
```typescript
// After editor setup:
loadFromShareLink();
```

### shellTemplate.ts — New Button

Add "Поделиться" button in the editor toolbar section, alongside "Сохранить", "Запустить", "Проверить".

## Runtime State Inventory

> This phase is NOT a rename/refactor/migration phase. It's a feature addition phase. Runtime state inventory is skipped. No stored data, live service config, OS-registered state, secrets, or build artifacts need migration.

**Step 2.5: SKIPPED (not a rename/refactor/migration phase)**

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| PlatformIO | Build & flash | ✓ | (existing) | — |
| ESP32-C3 toolchain | Build | ✓ | (existing) | — |
| LittleFS | Preset storage | ✓ | (ESP32 built-in) | — |
| Node.js (for frontend dev) | Frontend dev server | ✓ | (existing) | — |
| `navigator.clipboard` API | Share button | ✓ (browser) | (browser) | `document.execCommand('copy')` fallback for older browsers |

**Missing dependencies with no fallback:** None — all dependencies are in-tree or browser built-ins.

**Step 2.6: COMPLETE — no missing dependencies.**

## Common Pitfalls

### Pitfall 1: ESP32 WebServer PUT Body Handling
**What goes wrong:** `server_.arg("plain")` returns empty string for PUT requests, causing preset save to fail silently.
**Why it happens:** ESP32 WebServer library may only buffer request body for POST, not PUT. The body is available at a lower level but not exposed through the `arg()` API for PUT.
**How to avoid:** Use POST for preset save (add `POST /api/presets/<id>` route) as primary fix. Alternatively, switch to path-based PUT and test on real hardware.
**Warning signs:** Save returns HTTP 200 or 400 but preset doesn't appear in list after refresh. Check `parsePresetUpsertBody()` return value — it fails if `name` or `source` is empty.

### Pitfall 2: For-Loop Layer Explosion
**What goes wrong:** A `for` loop with large bounds generates hundreds of `CompiledLayer` instances, exhausting ESP32-C3 RAM.
**Why it happens:** Each `CompiledLayer` is ~40 bytes. 256 layers = ~10KB. ESP32-C3 has ~400KB SRAM but the heap is shared with WiFi, LittleFS, LED buffer, etc. Layer count above ~100 risks heap fragmentation.
**How to avoid:** Set a hard limit: `MAX_UNROLLED_LAYERS = 64` in config. Validate loop bounds at compile time: `(end - start) / step <= 64`. Emit diagnostic if exceeded.
**Warning signs:** Device crashes or reboots when loading a `for`-heavy effect. Watch for "heap allocation failed" in serial output.

### Pitfall 3: Frame Expression Division by Zero
**What goes wrong:** `frame = (t * 4) % 0` when a sprite has zero frames, causing NaN or crash.
**Why it happens:** Edge case: sprite with `frames` vector empty (single-frame fallback) but `frame` expression references frame count.
**How to avoid:** In Executor, guard: `if (sprite.frames.empty()) { use sprite.pixels (frame 0) }`. The modulo by 0 case is already handled in `evaluateNode()` for `kModulo` (returns 0 if denominator is 0). But explicitly check `frames.size() > 0` before using frame expression.
**Warning signs:** Flickering or blank sprite when `frame` expression evaluates to unexpected values.

### Pitfall 4: Backward Compatibility Breakage
**What goes wrong:** Existing `.lux` effects fail to parse or render after DSL changes.
**Why it happens:** Parser changes that don't handle old sprite format (single bitmap, no `frame` keyword), or Compiler changes that require new fields.
**How to avoid:** Run ALL existing test cases after each pipeline change. The test suite (`test_dsl_parser`, `test_dsl_executor`) should pass without modification. Add explicit backward-compat tests: "sprite without frame keyword compiles and renders correctly."
**Warning signs:** Any existing test failure. Any change to `SpriteDeclaration` that removes the `bitmap` field.

### Pitfall 5: Loop Variable Name Collision
**What goes wrong:** Loop variable `i` shadows a future built-in variable or collides with another loop variable in nested loops.
**Why it happens:** String substitution in compile-time unrolling is naive — it replaces ALL occurrences of the variable name in expressions.
**How to avoid:** Validate that loop variable name is not `t`, `dt`, `x`, `y`, `nx`, `ny`, `temp`, `humidity`, or any function name. For nested loops, inner loop variable must differ from outer. Use a set of reserved words to check against.
**Warning signs:** Unexpected expression evaluation results when loop variable name clashes with built-in.

## Code Examples

### Multi-Frame Sprite DSL
```
effect "mario_run"

sprite mario {
    frame walk1 {
        bitmap """
        ...##...
        .######.
        .##..##.
        .######.
        .##.##..
        .##.##..
        ..####..
        .##..##.
        """
    }
    frame walk2 {
        bitmap """
        ...##...
        .######.
        .##..##.
        .######.
        .##.##..
        ..####..
        .##..##.
        ##....##
        """
    }
}

layer runner {
    use mario
    color rgb(255, 255, 255)
    x = (t * 8) % 32
    y = 4
    scale = 1
    frame = (t * 4) % 2
    visible = 1
}
```

### For Loop DSL (Snake)
```
effect "snake"

sprite dot {
    bitmap """
    #
    """
}

for i = 0; i < 8; i = i + 1 {
    layer segment {
        use dot
        color rgb(0, 255 - i * 25, 0)
        x = (t * 3 + i * 3) % 32
        y = 8
        scale = 1
        visible = 1
    }
}
```

### Per-Pixel Plasma (No New Features Needed)
```
effect "plasma"

sprite fullscreen {
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

layer plasma {
    use fullscreen
    color hsv(
        sin(nx * 8 + t * 0.7) * 180 + sin(ny * 6 + t * 0.5) * 180,
        1,
        0.5 + sin(nx * 12 + ny * 8 + t) * 0.5
    )
    x = 0
    y = 0
    visible = 1
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Single bitmap per sprite | Multi-frame sprite with named frames | Phase 5 | Enables sprite animation; backward compatible |
| Declarative layers only | Top-level `for` loops for layer generation | Phase 5 | Enables multi-segment effects (Snake, Scrolling Text) |
| Manual copy-paste sharing | URL-safe base64 share links | Phase 5 | One-click effect sharing |

**Deprecated/outdated:**
- `SpriteDeclaration::bitmap` (single field): Still supported for backward compat, but multi-frame sprites use `frames` vector. Planner-Service pattern unchanged.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | ESP32 WebServer `server_.arg("plain")` doesn't work for PUT requests | Preset Save Bug Analysis | MEDIUM — If wrong, the bug is elsewhere (e.g., JSON parsing issue, route mismatch). Mitigation: also test path-based PUT and POST fallback. |
| A2 | All demo `for` loops have compile-time constant bounds | DSL Grammar Changes | LOW — Demo effects are hand-written and use literal bounds (e.g., `i < 8`). If we later support dynamic bounds, runtime loop evaluation would be needed, which is deferred. |
| A3 | `navigator.clipboard.writeText()` is available in target browsers | Base64 Share Link | LOW — All modern browsers support it. Fallback to `document.execCommand('copy')` for older browsers is trivial. |
| A4 | LittleFS `/presets/` directory is empty on first boot | Factory Preset Seeding | LOW — Standard ESP32 LittleFS behavior. If the directory doesn't exist, `list()` returns empty vector. |
| A5 | 64 unrolled layers is a safe upper bound for ESP32-C3 heap | Common Pitfalls | LOW — Conservative estimate. Each layer ~40 bytes × 64 = 2.5KB. Heap is ~200KB+ after static allocations. |

## Open Questions

1. **Nested `for` loops support?**
   - What we know: D-03 specifies `for` syntax but doesn't mention nested loops. No demo effect requires nesting.
   - What's unclear: Should the implementation allow nesting proactively or block it with a diagnostic?
   - Recommendation: **Block nested loops in Phase 5.** Emit diagnostic "Вложенные циклы for не поддерживаются в v1". Can be relaxed later if needed.

2. **`for` loop comparison operators — only `<` or also `<=`, `>`, `>=`?**
   - What we know: D-03 specifies `<` in the syntax. All demo effects use `<` (0 to N exclusive).
   - What's unclear: Should we support `<=` for inclusive ranges?
   - Recommendation: **Support `<` and `<=` only.** `>` and `>=` add parsing complexity for no demo benefit. `<=` is useful for "0 to N inclusive" patterns.

3. **Should the share link include the effect name as a fragment for readability?**
   - What we know: D-10 specifies base64-encoded DSL code. Pure base64 URLs are opaque.
   - What's unclear: Adding `#nyan-cat` fragment for human readability without affecting decoding.
   - Recommendation: **Add effect name as URL fragment.** `?code=BASE64#nyan-cat`. The fragment is ignored by the decoder but visible to humans.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Unity (PlatformIO unit testing) + Native frontend dev server manual testing |
| Config file | `platformio.ini` (existing `test/` environments) |
| Quick run command | `platformio test -e native --filter "test_dsl_*" -v` |
| Full suite command | `platformio test -e native -v` |

### Phase Requirements → Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| DSL-01 | Multi-frame sprite parses and renders correct frame | unit | `platformio test -e native --filter "test_dsl_parser" -v` | ❌ Wave 0 (extend existing) |
| DSL-01 | Frame selection expression changes rendered frame | unit | `platformio test -e native --filter "test_dsl_executor" -v` | ❌ Wave 0 (extend existing) |
| DSL-02 | `for` loop parses without diagnostics | unit | `platformio test -e native --filter "test_dsl_parser" -v` | ❌ Wave 0 |
| DSL-02 | `for` loop unrolls to correct number of layers | unit | `platformio test -e native --filter "test_dsl_executor" -v` | ❌ Wave 0 |
| DSL-02 | Backward compat: old `.lux` files parse and render unchanged | regression | `platformio test -e native -v` | ✅ Existing tests must stay green |
| DSL-03 | Demo presets appear in preset list after first boot | integration | Manual (flash + web UI check) | N/A (manual UAT) |
| DSL-03 | Demo snippets load into editor from "Идеи" panel | smoke | Manual (dev server + click) | N/A (manual UAT) |
| D-11 | Save button creates new preset visible in list | integration | Manual (flash + save + list refresh) | N/A (manual UAT) |
| D-10 | Share button copies valid base64 URL to clipboard | unit/smoke | Manual (browser dev tools) | N/A (manual UAT) |

### Sampling Rate
- **Per task commit:** `platformio test -e native --filter "test_dsl_*" -v`
- **Per wave merge:** `platformio test -e native -v`
- **Phase gate:** Full suite green + manual UAT on physical device with all 8 demo effects

### Wave 0 Gaps
- [ ] `test/test_dsl_parser/test_main.cpp` — add tests for: multi-frame sprite parsing, `for` loop parsing, backward compat of old sprite format, `frame` field in layer
- [ ] `test/test_dsl_executor/test_main.cpp` — add tests for: sprite frame selection by expression, `for` loop unrolling producing correct layer count, loop variable substitution in expressions
- [ ] No new test files needed; extend existing test files

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | No | Local network only; no auth in v1 |
| V3 Session Management | No | Stateless HTTP API |
| V4 Access Control | No | Trusted local network |
| V5 Input Validation | **Yes** | DSL source validated by Lexer→Parser→Compiler pipeline before execution; JSON body validated by ArduinoJson schema in `parsePresetUpsertBody()`; base64 input validated by `atob()` failure handling |
| V6 Cryptography | No | No cryptographic operations in this phase |

### Known Threat Patterns for Lux DSL

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Malformed DSL source causing parser crash | Denial of Service | Parser returns `false` + diagnostics on any unexpected token; no `abort()` or unguarded allocation |
| Excessively large for-loop bounds exhausting heap | Denial of Service | Hard limit `MAX_UNROLLED_LAYERS = 64`; compiler checks bounds before unrolling |
| Base64-encoded share link with injected malicious code | Spoofing | Decoded DSL must pass `validateSource()` before execution; no automatic execution on decode |
| Preset JSON with oversized fields | Denial of Service | ArduinoJson `StaticJsonDocument` with fixed capacity (`kPresetRequestCapacity = 2048`); parse fails on overflow |
| Recursive expression tree overflow | Denial of Service | `kMaxExpressionDepth = 64` already enforced in `evaluateNode()` |

## Sources

### Primary (HIGH confidence)
- `include/live/dsl/Ast.h` — current AST structures verified in codebase
- `include/live/dsl/Token.h` — current token types verified
- `include/live/runtime/CompiledProgram.h` — current bytecode structures verified
- `include/live/runtime/Compiler.h` — compiler interface verified
- `include/live/runtime/Executor.h` — executor interface verified
- `include/live/runtime/LiveProgramService.h` — service orchestration verified
- `include/live/PresetModel.h` — preset data model verified
- `include/live/PresetRepository.h` — preset CRUD verified
- `include/AppConfig.h` — hardware constants (kLogicalWidth=16, kLogicalHeight=32 with XY-swap) verified
- `include/FrameBuffer.h` — drawing primitives verified
- `src/live/dsl/Lexer.cpp` — lexer implementation verified
- `src/live/dsl/Parser.cpp` — parser implementation verified
- `src/live/runtime/Compiler.cpp` — compiler implementation (expression compilation, sprite compilation) verified
- `src/live/runtime/Executor.cpp` — executor implementation (render loop, pixel rendering, XY-swap) verified
- `src/web/LampWebServer.cpp` — route registration and preset save paths verified
- `src/web/PresetApi.cpp` — preset API handlers verified
- `src/web/LiveApi.cpp` — live API handlers verified
- `src/live/PresetRepository.cpp` — preset storage verified
- `frontend/src/main.ts` — save function, share function (to be added), event binding verified
- `frontend/src/editor/snippets.ts` — existing snippets verified
- `frontend/src/editor/help.ts` — existing help items verified
- `frontend/src/editor/luxHighlight.ts` — existing highlighting verified
- `test/test_dsl_parser/test_main.cpp` — existing parser tests verified
- `test/test_dsl_executor/test_main.cpp` — existing executor tests verified
- `src/main.cpp` — factory preset seeding location verified
- `docs/DSL.md` — Lux language reference verified

### Secondary (MEDIUM confidence)
- ESP32 WebServer library behavior with PUT — inferred from code analysis; needs device verification [CITED: codebase comparison of POST vs PUT body handling]

### Tertiary (LOW confidence)
- None — all claims traceable to codebase analysis or explicitly marked [ASSUMED]

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — no new external dependencies; all in-tree
- Architecture: HIGH — pipeline extension pattern well-understood from existing code
- Pitfalls: MEDIUM — PUT body handling needs device verification (A1); other pitfalls verified
- DSL grammar: HIGH — concrete changes mapped to specific files and structures
- Frontend changes: HIGH — existing code patterns (save, snippets, highlighting) provide clear templates
- Demo effects: MEDIUM — pixel art design needs iteration on physical device; DSL code quality depends on author skill

**Research date:** 2026-06-02
**Valid until:** 2026-06-16 (stable domain; DSL architecture unlikely to change within 2 weeks)
