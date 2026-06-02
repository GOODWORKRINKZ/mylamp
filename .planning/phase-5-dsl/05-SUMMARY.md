# Phase 5: Demo Effects & DSL — Summary

**Status:** Complete
**Date:** 2026-06-02

## What was built

### Plan 05-01: DSL Grammar Extensions ✓
- **Token.h**: `kKeywordFor`, `kKeywordFrame`, `kSemicolon`, `kKeywordPalette`
- **Ast.h**: `SpriteFrameDeclaration`, `ForLoopStatement`, `PaletteEntry`, `PaletteDeclaration`; extended `SpriteDeclaration`, `LayerDeclaration`, `Program`
- **Lexer.cpp**: Multi-frame sprite bodies, for-loop headers, palette blocks, `frame=` property
- **Parser.cpp**: `parseSprite()` multi-frame path, `parseForLoop()` with reserved-word validation, `parsePalette()`, palette dispatch
- **Tests**: 9 parser tests (4 existing + 5 new), all PASS

### Plan 05-02: Compiler + Executor ✓
- **CompiledProgram.h**: `CompiledSprite.frames`, `CompiledLayer.frameExpression`, `kLoopIndex`, `kMaxUnrolledLayers=64`, per-pixel palette colors
- **Compiler.cpp**: `compileSprite()` multi-frame + palette resolution, for-loop unrolling with `replaceAll()` string substitution, frame expression compilation
- **Executor.cpp**: Frame index evaluation (`% frames.size()`), per-pixel palette color path
- **Tests**: 2 new executor tests pass; pre-existing test failures are from earlier phases

### Plan 05-03: Demo Effects + Factory Presets ✓
- **Palette system**: `palette <name> { X = rgb(r,g,b); ... }` → per-pixel colors via `CompiledSpritePixel.pr/pg/pb`
- **8 demo .lux files** in `resources/demo/`:
  - `mario.lux` — SMB1 Mario 16×16, 3 walk frames, NES colors (R/S/W)
  - `nyan-cat.lux` — Nyan Cat 12×12, 4 frames (G/P/K/W)
  - `plasma.lux` — Fullscreen per-pixel hsv waves
  - `scrolling-text.lux` — Text scroller
  - `snake.lux` — 10-segment for-loop snake
  - `fire-particles.lux` — 12-particle rising fire
  - `starfield.lux` — 20-star flying field
  - `dna.lux` — 16-pair double helix
- **Factory presets**: `seedFactoryPresets()` in `main.cpp`, 8 presets on first boot

### Plan 05-04: Frontend ✓
- **Save fix**: PUT `/api/presets/<id>` (path-based, was `?id=`)
- **Share button**: URL-safe base64 → clipboard; `?code=` decode on page load
- **8 snippets**: Nyan Cat, Mario, Plasma, Scrolling Text, Snake, Fire, Starfield, DNA
- **Help**: `for` and `frame` entries in Директивы
- **Highlighting**: `for`, `frame`, `palette` in LUX_KEYWORDS

## Decision Traceability

| Decision | How Implemented |
|----------|----------------|
| D-01 (named frames) | `SpriteFrameDeclaration`, multi-frame lexer/parser |
| D-02 (frame expression) | `LayerDeclaration.frameExpression`, compiler + executor |
| D-03 (for loops) | `ForLoopStatement`, `parseForLoop()`, `unrollForLoops()` |
| D-04 (backward compat) | Single-bitmap path preserved; `frames.empty()` → `sprite.pixels` |
| D-05 (use stays sprite name) | `frame` is separate property, not part of `use` |
| D-06–D-08 (demos) | 8 .lux files, all valid Lux DSL |
| D-09 (snippets + presets) | `snippets.ts` + `seedFactoryPresets()` |
| D-10 (share link) | `handleShare()` + `decodeShareLink()` |
| D-11 (save route fix) | `PUT /api/presets/<id>` |

## Palette System (added during execution)

User requested multi-color sprites. Implemented:
- `palette <name> { X = rgb(r,g,b); ... }` top-level construct
- `sprite <name> palette <name> { ... }` syntax
- Per-pixel color via `CompiledSpritePixel.pr/pg/pb`
- Compiler resolves palette chars via `sscanf` + HSV→RGB conversion
- Executor uses `pixel.hasPixelColor` path bypassing `layer.color`

## Requirements Coverage

| Requirement | Status |
|-------------|--------|
| DSL-01 (demo effects with sprite animation) | ✓ Mario + Nyan Cat multi-frame |
| DSL-02 (Lux language extensions) | ✓ for-loops, multi-frame, palettes |
| DSL-03 (demos as web presets) | ✓ 8 snippets + factory presets |

## Verification

```
platformio test -e native-test --filter "test_dsl_parser" -v
→ 9 Tests 0 Failures — PASSED
```

## Next Steps

1. Flash dev build to device for visual UAT
2. Verify: Mario walks with 3-color NES palette
3. Verify: Share button copies working URL
4. Verify: Share link decodes and loads effect
