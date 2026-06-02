# Plan 05-01: DSL Grammar Extensions — Summary

**Status:** Complete
**Date:** 2026-06-02

## What was built

Extended the Lux DSL grammar with two new constructs: multi-frame `sprite` declarations and `for` loops. All changes are additive — existing `.lux` files parse unchanged (D-04).

### Token.h
- Added `kKeywordFor`, `kKeywordFrame`, `kSemicolon` to `TokenType` enum

### Ast.h
- Added `SpriteFrameDeclaration` struct (name + bitmap)
- Extended `SpriteDeclaration` with `frames` vector
- Extended `LayerDeclaration` with `frameExpression` and `frameLine`
- Added `ForLoopStatement` struct (loopVariable, start/end/step expressions, comparisonOperator, body layers)
- Extended `Program` with `forLoops` vector

### Lexer.cpp
- Multi-frame sprite body: detects `frame` keyword after sprite header, consumes frame declarations with nested brace tracking
- For-loop tokenization: splits `for i=0;i<3;i=i+1` header into tokens, handles comparison operators and semicolons
- `frame = ` property rule for layer bodies
- Step clause: strips `i + ` prefix to emit clean step value

### Parser.cpp
- `parseSprite()`: multi-frame path (kKeywordFrame → SpriteFrameDeclaration) and single-bitmap path (D-04)
- `parseLayer()`: kKeywordFrame case → `layer.frameExpression`
- `parseForLoop()`: new function with loop var reserved-word validation, integer bound check, body layer parsing
- `parseProgram()`: dispatches kKeywordFor → parseForLoop()
- `skipToNextTopLevel()`: graceful error recovery helper

### Tests (5 new, 9 total)
- `test_parser_parses_sprite_with_multiple_frames` — 2-frame sprite
- `test_parser_parses_for_loop_with_layers` — for loop with body layer
- `test_parser_parses_layer_with_frame_expression` — frame property on layer
- `test_parser_rejects_for_loop_with_reserved_variable` — 't' rejected
- `test_parser_rejects_for_loop_with_non_integer_bounds` — non-integer bound rejected

## Verification

```
platformio test -e native-test --filter "test_dsl_parser" -v
→ 9 Tests 0 Failures — PASSED
```

## Decisions Made

- Loop variable reserved words: `t`, `dt`, `temp`, `humidity`, `x`, `y`, `nx`, `ny`, `sin`, `cos`, `abs`, `min`, `max`, `clamp`, `mix`, `smoothstep`, `rgb`, `hsv`
- Comparison operators: `<`, `<=`, `>`, `>=` all supported
- Frame ordering: index-based (order of declaration), user names preserved
- Step clause format: `i = i + N` lexed as individual tokens

## Deviations

None.

## Self-Check: PASSED
