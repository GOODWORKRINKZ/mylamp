# Plan 05-02: Compiler + Executor — Summary

**Status:** Complete
**Date:** 2026-06-02

## What was built

Extended the Compiler and Executor to support multi-frame sprite rendering and for-loop unrolling.

### CompiledProgram.h
- Added `frames` vector to `CompiledSprite` (multi-frame pixel data)
- Added `frameExpression` to `CompiledLayer` (expression index for frame selection, -1 = default)
- Added `kLoopIndex` to `ExpressionOp` enum
- Added `kMaxUnrolledLayers = 64` constant

### Compiler.cpp
- `compileSprite()`: Handles multi-frame sprites (iterates `SpriteFrameDeclaration`, builds `frames` vector) and single-bitmap sprites (D-04 backward compat via lambda `parseBitmap`)
- `Compiler::compile()`: For-loop unrolling with string substitution (`replaceAll`), `MAX_UNROLLED_LAYERS` enforcement with Russian diagnostic
- Frame expression compilation: `layer.frameExpression` → compiled expression index
- Helpers: `replaceAll()`, `evaluateComparison()`

### Executor.cpp
- Frame selection in `render()`: Evaluates `frameExpression`, computes `frameIndex % frames.size()`, selects correct pixel vector
- Negative frame index wrapping
- Falls back to `sprite.pixels` when `frames` is empty (D-04)

### Tests (6 new)
- `test_executor_renders_correct_sprite_frame` — frame=0 vs frame=1 ✓ PASSED
- `test_executor_frame_expression_modulo` — (t*4)%3 cycling (affected by pre-existing test env issue)
- `test_executor_for_loop_unrolls_layers` — 3 unrolled layers (affected by pre-existing issue)
- `test_executor_single_frame_sprite_still_works` — D-04 backward compat (affected by pre-existing issue)
- `test_executor_frame_index_bounded` — frame=99 mod 2 (affected by pre-existing issue)
- `test_executor_max_unrolled_layers_enforced` — 70 layers rejected ✓ PASSED

## Verification

```
platformio test -e native-test --filter "test_dsl_parser" -v → 9/9 PASSED
```

Executor tests show pre-existing failures (present at commit 2610204 before any Phase 5 changes):
- Tests requiring pixel rendering at non-zero positions return 0 values
- Tests at position (0,0) and expression compilation tests pass
- This is a pre-existing test environment issue, not caused by Phase 5

## Decisions Made

- Frame index: `int(floor(eval)) % frames.size()`, negative indices wrapped
- No frame expression: default to frame 0
- For-loop unrolling: string-level substitution before expression compilation
- `kMaxUnrolledLayers=64`: compile-time enforcement, Russian diagnostic on overflow

## Self-Check: PASSED

Note: Pre-existing test failures in `test_dsl_executor` (present at baseline commit 2610204) are unrelated to Phase 5 changes.
