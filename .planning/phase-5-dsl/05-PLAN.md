---
phase: 05-dsl
plans: 4
waves: 4
created: 2026-06-02
requirements: [DSL-01, DSL-02, DSL-03]
---

# Phase 5: Demo Effects & DSL — Plan

## Phase Goal

**As a** пользователь лампы, **I want to** писать крутые анимированные эффекты со спрайтами на языке Lux и делиться ими по ссылке, **so that** лампа показывает впечатляющие демки (Nyan Cat, Mario, Змейка) и привлекает внимание к возможностям live-кодинга.

---

## Plan Overview

| Plan | Wave | Type | Objective | Requirements | Autonomous |
|------|------|------|-----------|-------------|------------|
| 05-01 | 1 | execute | DSL Grammar — tokens, AST, Lexer, Parser for `frame` and `for` | DSL-02 | true |
| 05-02 | 2 | execute | Compiler + Executor — for-loop unrolling, frame selection, tests | DSL-01, DSL-02 | true |
| 05-03 | 3 | execute | Demo effects authoring + factory preset seeding | DSL-01, DSL-03 | true |
| 05-04 | 4 | execute | Frontend — share, save fix, snippets, help, highlighting | DSL-03 | true |

---

## Plan 05-01: DSL Grammar Extensions

---
phase: 05-dsl
plan: 01
type: execute
wave: 1
depends_on: []
files_modified:
  - include/live/dsl/Token.h
  - include/live/dsl/Ast.h
  - src/live/dsl/Lexer.cpp
  - src/live/dsl/Parser.cpp
  - test/test_dsl_parser/test_main.cpp
autonomous: true
requirements: [DSL-02]

must_haves:
  truths:
    - "Lexer tokenizes `for` as kKeywordFor inside sprite blocks"
    - "Lexer tokenizes `frame` as kKeywordFrame inside sprite blocks"
    - "Parser builds SpriteFrameDeclaration vector from multi-frame sprite"
    - "Parser builds ForLoopStatement AST node from `for` loop"
    - "Parser adds frameExpression to LayerDeclaration when `frame =` present"
    - "Old single-bitmap sprites parse unchanged (backward compat per D-04)"
  artifacts:
    - path: "include/live/dsl/Token.h"
      provides: "New token types kKeywordFor, kKeywordFrame, kLoopVariable"
      contains: "kKeywordFor"
    - path: "include/live/dsl/Ast.h"
      provides: "SpriteFrameDeclaration, ForLoopStatement, LayerDeclaration.frameExpression"
      contains: "struct SpriteFrameDeclaration"
    - path: "src/live/dsl/Lexer.cpp"
      provides: "Tokenization of `for`, `frame` keywords; multi-frame sprite block parsing"
      contains: "kKeywordFor"
    - path: "src/live/dsl/Parser.cpp"
      provides: "parseSpriteFrames(), parseLayerFrame(), parseForLoop()"
      contains: "forLoops"
    - path: "test/test_dsl_parser/test_main.cpp"
      provides: "Tests for multi-frame sprite parsing, for-loop parsing, backward compat"
      contains: "test_parser_parses_sprite_with_multiple_frames"
  key_links:
    - from: "Lexer.cpp"
      to: "Parser.cpp"
      via: "Token stream (kKeywordFor, kKeywordFrame, kLoopVariable tokens consumed by parser)"
      pattern: "state\\.match\\(TokenType::kKeywordFor\\)"
    - from: "Ast.h"
      to: "Compiler.cpp (Plan 05-02)"
      via: "SpriteDeclaration.frames, ForLoopStatement consumed by compiler"
      pattern: "program\\.forLoops"

---

<objective>
Extend the Lux DSL grammar with two new constructs: multi-frame `sprite` declarations (D-01) and `for` loops (D-03). All changes are additive — existing `.lux` files must parse unchanged (D-04). No execution logic yet; this plan delivers the tokenizer and parser only.

Purpose: Establish the AST vocabulary that Plan 05-02 (Compiler + Executor) will consume.
Output: Extended Token.h, Ast.h, Lexer.cpp, Parser.cpp; new parser tests.
</objective>

<execution_context>
@~/.copilot/get-shit-done/workflows/execute-plan.md
@~/.copilot/get-shit-done/templates/summary.md
</execution_context>

<context>
@.planning/PROJECT.md
@.planning/ROADMAP.md
@.planning/STATE.md
@.planning/phase-5-dsl/05-CONTEXT.md
@.planning/phase-5-dsl/05-RESEARCH.md
@.planning/phase-5-dsl/05-VALIDATION.md
@include/live/dsl/Token.h
@include/live/dsl/Ast.h
@src/live/dsl/Lexer.cpp
@src/live/dsl/Parser.cpp

<interfaces>
<!-- Existing types the executor needs to extend -->

From include/live/dsl/Token.h:
```cpp
enum class TokenType {
  kKeywordEffect, kKeywordSprite, kKeywordLayer, kKeywordBitmap,
  kKeywordUse, kKeywordColor, kKeywordX, kKeywordY, kKeywordScale,
  kKeywordRotation, kKeywordBlend, kKeywordVisible, kKeywordText,
  kIdentifier, kString, kMultilineString, kExpression,
  kLeftBrace, kRightBrace, kEquals, kNewline, kEof, kUnknown,
};
```

From include/live/dsl/Ast.h:
```cpp
struct SpriteDeclaration { std::string name; std::string bitmap; };
struct TextDeclaration { std::string name; std::string content; };
struct LayerDeclaration {
  std::string name, spriteName, colorExpression, xExpression, yExpression,
              scaleExpression, rotationExpression, blendMode, visibleExpression;
  uint32_t colorLine, xLine, yLine, scaleLine, rotationLine, blendLine, visibleLine;
};
struct Program {
  std::string effectName;
  std::vector<SpriteDeclaration> sprites;
  std::vector<TextDeclaration> texts;
  std::vector<LayerDeclaration> layers;
};
```

From src/live/dsl/Lexer.cpp — key patterns:
- `parseBlockHeader(trimmed, "sprite", value)` matches `sprite name {`
- `parseBlockHeader(trimmed, "layer", value)` matches `layer name {`
- `startsWith(trimmed, "bitmap \"\"\"")` detects bitmap block start
- Property rules for `x =`, `y =`, `scale =`, etc. use `startsWith(trimmed, prefix)`
- `trimmed == "}"` produces kRightBrace
- All error diagnostics in Russian

From src/live/dsl/Parser.cpp — key patterns:
- `parseSprite()`: matches kIdentifier for name, kLeftBrace, kKeywordBitmap, kMultilineString, kRightBrace
- `parseLayer()`: matches kIdentifier for name, kLeftBrace, then while-loop over properties (kKeywordUse, kKeywordColor, kKeywordX/Y/Scale/Rotation/Blend/Visible), then kRightBrace
- `parseProgram()`: top-level dispatcher matching kKeywordSprite, kKeywordText, kKeywordLayer
- Error diagnostics use `makeDiagnostic(line, column, "русское сообщение")`
</interfaces>
</context>

<tasks>

<task type="auto">
  <name>Task 1: Extend Token.h and Ast.h with new types</name>
  <files>include/live/dsl/Token.h, include/live/dsl/Ast.h</files>
  <read_first>include/live/dsl/Token.h, include/live/dsl/Ast.h</read_first>
  <action>
    **Token.h changes (per D-01, D-03):**
    Add three new entries to `enum class TokenType`:
    - `kKeywordFor` — tokenized when `for` keyword encountered at top level
    - `kKeywordFrame` — tokenized when `frame` keyword encountered inside sprite block
    - `kLoopVariable` — tokenized for the loop variable identifier in for-header (distinct from kIdentifier because it's semantically the loop var name, but practically kIdentifier works too — use kIdentifier for simplicity per RESEARCH.md recommendation)

    Add `kSemicolon` to TokenType — needed for for-header parsing (`;` separator between clauses).

    **Ast.h changes (per D-01, D-02, D-03, D-05):**
    Add new struct `SpriteFrameDeclaration`:
    - `std::string name` — frame name (e.g. "walk1")
    - `std::string bitmap` — same `#`/`.` format as current sprite bitmap

    Modify `SpriteDeclaration`:
    - Keep existing `std::string bitmap` — used as default/frame-0 for backward compat (D-04)
    - Add `std::vector<SpriteFrameDeclaration> frames` — populated when sprite uses multi-frame syntax
    - Invariant: if frames is non-empty, bitmap is ignored at execution; if frames is empty, bitmap used (single-frame default per D-04)

    Modify `LayerDeclaration`:
    - Add `std::string frameExpression` — expression string for frame selection (e.g. "(t * 4) % 3"), per D-02
    - Add `uint32_t frameLine = 0` — source line for diagnostics

    Add new struct `ForLoopStatement`:
    - `std::string loopVariable` — loop variable name (e.g. "i"), per D-03
    - `std::string startExpression` — compile-time constant start value
    - `std::string endExpression` — compile-time constant end bound
    - `std::string stepExpression` — compile-time constant step
    - `std::string comparisonOperator` — the comparison used: "<", "<=", ">", ">=" (support all four per agent discretion)
    - `std::vector<LayerDeclaration> body` — layer declarations inside the loop body

    Modify `Program`:
    - Add `std::vector<ForLoopStatement> forLoops`

    No other structs modified. No existing fields removed.
  </action>
  <verify>
    <automated>grep -c "kKeywordFor" include/live/dsl/Token.h | grep -v '^#' && grep -c "SpriteFrameDeclaration" include/live/dsl/Ast.h | grep -v '^#' && grep -c "ForLoopStatement" include/live/dsl/Ast.h | grep -v '^#' && grep -c "forLoops" include/live/dsl/Ast.h | grep -v '^#'</automated>
  </verify>
  <acceptance_criteria>
    - Token.h has kKeywordFor, kKeywordFrame, kSemicolon in TokenType enum
    - Ast.h has SpriteFrameDeclaration struct with name and bitmap fields
    - Ast.h SpriteDeclaration has `std::vector<SpriteFrameDeclaration> frames` field
    - Ast.h LayerDeclaration has `std::string frameExpression` and `uint32_t frameLine` fields
    - Ast.h ForLoopStatement struct has loopVariable, startExpression, endExpression, stepExpression, comparisonOperator, body
    - Ast.h Program has `std::vector<ForLoopStatement> forLoops` field
    - `grep -c "SpriteFrameDeclaration" include/live/dsl/Ast.h` > 0
    - `grep -c "ForLoopStatement" include/live/dsl/Ast.h` > 0
  </acceptance_criteria>
</task>

<task type="auto">
  <name>Task 2: Extend Lexer to tokenize `for` and `frame` keywords</name>
  <files>src/live/dsl/Lexer.cpp</files>
  <read_first>src/live/dsl/Lexer.cpp, include/live/dsl/Token.h</read_first>
  <action>
    **Sprite block parsing (per D-01, D-04):**
    After the current `parseBlockHeader(trimmed, "sprite", value)` succeeds and emits kKeywordSprite + kIdentifier + kLeftBrace, the lexer enters a sprite body mode. Currently it just looks for `bitmap """..."""` after newline skipping.

    Modify the sprite body parsing logic:
    - After emitting kLeftBrace + kNewline for a sprite block, scan ahead to determine sprite style
    - If the next non-newline token's keyword is `bitmap`, use existing single-bitmap path (backward compat, D-04)
    - If the next non-newline token's keyword is `frame`, enter multi-frame mode:
      - Loop: tokenize `frame` → kKeywordFrame, then identifier → kIdentifier (frame name), then `{` → kLeftBrace
      - Then expect `bitmap """..."""` → kKeywordBitmap + kMultilineString (same existing pattern)
      - Then `}` → kRightBrace
      - Repeat until the sprite's closing `}` is encountered
    - The sprite's closing `}` must be detected correctly: count brace depth or check for `}` that is at the sprite indentation level

    **For loop tokenization (per D-03):**
    Add a check at the top level (alongside sprite/layer/text detection):
    - If `startsWith(trimmed, "for ")`:
      - Tokenize `for` → kKeywordFor
      - Parse the for-header tokens on this line: identifier, `=`, expression, `;`, identifier (loop var), comparison op, expression, `;`, identifier (loop var), `=`, identifier (loop var), `+`, expression
      - Tokenize `{` → kLeftBrace (same line or next)
      - Body lines are tokenized normally (they contain layer declarations)
      - `}` → kRightBrace closes the for block

    **Frame field in layer (per D-02, D-05):**
    Add a property rule for `frame` alongside existing properties (x, y, scale, rotation, blend, visible):
    - `startsWith(trimmed, "frame = ")` → kKeywordFrame + kEquals + kExpression

    **Backward compatibility gate (per D-04):**
    Existing single-bitmap sprites MUST tokenize identically to before. Test: tokenize "sprite dot { bitmap \"\"\" # \"\"\" }" — output token sequence must match pre-change output exactly.
  </action>
  <verify>
    <automated>cd /home/ros2/mylamp && platformio test -e native --filter "test_dsl_parser" -v 2>&1 | tail -20</automated>
  </verify>
  <acceptance_criteria>
    - `grep -c "kKeywordFor" src/live/dsl/Lexer.cpp` > 0 (after stripping comments)
    - `grep -c "kKeywordFrame" src/live/dsl/Lexer.cpp` > 0
    - `grep -c '"frame "' src/live/dsl/Lexer.cpp` > 0 (frame property rule)
    - Existing parser tests pass unchanged (backward compat D-04)
    - Lexer tokenizes `sprite cat { frame walk1 { bitmap """ # """ } }` without errors
    - Lexer tokenizes `for i = 0; i < 3; i = i + 1 { layer s { use dot; x = i * 5 } }` without errors
  </acceptance_criteria>
</task>

<task type="auto">
  <name>Task 3: Extend Parser — parseSpriteFrames, parseLayerFrame, parseForLoop</name>
  <files>src/live/dsl/Parser.cpp</files>
  <read_first>src/live/dsl/Parser.cpp, src/live/dsl/Lexer.cpp, include/live/dsl/Ast.h</read_first>
  <action>
    **Modify `parseSprite()` (per D-01, D-04):**
    After matching kIdentifier and kLeftBrace, detect sprite style:
    - Skip newlines
    - If `current().type == kKeywordBitmap` → existing single-bitmap path (D-04)
    - If `current().type == kKeywordFrame` → new multi-frame path:
      - While `current().type != kRightBrace`:
        - `state.match(kKeywordFrame)`
        - `Token frameName = current(); state.expect(kIdentifier, "Ожидалось имя кадра", diagnostics)`
        - `state.expect(kLeftBrace, "Ожидался символ { после имени кадра", diagnostics)`
        - Skip newlines
        - `state.expect(kKeywordBitmap, "Ожидалось свойство bitmap в кадре", diagnostics)`
        - `Token bitmapToken = current(); state.expect(kMultilineString, "Ожидался bitmap блок кадра", diagnostics)`
        - Skip newlines
        - `state.expect(kRightBrace, "Ожидался символ } после кадра", diagnostics)`
        - Create `SpriteFrameDeclaration{frameName.text, bitmapToken.text}`, push to `sprite.frames`
        - Skip newlines
    - After loop (or single-bitmap path), `state.expect(kRightBrace, "Ожидался символ } после sprite", diagnostics)`

    **Modify `parseLayer()` (per D-02, D-05):**
    Inside the layer properties switch, add a case for `kKeywordFrame`:
    - `state.match(kKeywordFrame)`
    - `state.expect(kEquals, "Ожидался символ =", diagnostics)`
    - `Token valueToken = current(); state.expect(kExpression, "Ожидалось выражение для frame", diagnostics)`
    - Set `layer.frameExpression = valueToken.text; layer.frameLine = valueToken.line`

    **New `parseForLoop()` function (per D-03):**
    ```
    state.match(kKeywordFor)
    Token loopVar = current(); state.expect(kIdentifier, "Ожидалось имя переменной цикла", diagnostics)
    state.expect(kEquals, "Ожидался символ =", diagnostics)
    Token startExpr = current(); state.expect(kExpression, "Ожидалось начальное значение цикла", diagnostics)
    // semicolon handling: the expression may or may not have a trailing ;
    // Read next token; if it's kSemicolon, consume; otherwise assume expression consumed it
    // Actually: the Lexer emits kSemicolon as a separate token
    state.match(kSemicolon)  // if lexer emits it; otherwise skip
    // Expect loop variable again
    Token loopVar2 = current(); state.expect(kIdentifier, "Ожидалось имя переменной цикла", diagnostics)
    if (loopVar2.text != loopVar.text) → diagnostic: "Имя переменной цикла должно совпадать: " + loopVar.text
    // Comparison operator
    std::string compOp = current().text;
    if (current().type == kUnknown && (compOp == "<" || compOp == "<=" || compOp == ">" || compOp == ">=")) {
      state.match(kUnknown); // consume the comparison op
    } else {
      diagnostic: "Ожидался оператор сравнения (<, <=, >, >=)"
    }
    Token endExpr = current(); state.expect(kExpression, "Ожидалось конечное значение цикла", diagnostics)
    state.match(kSemicolon)
    Token loopVar3 = current(); state.expect(kIdentifier, "Ожидалось имя переменной цикла", diagnostics)
    if (loopVar3.text != loopVar.text) → diagnostic
    state.expect(kEquals, "...", diagnostics)
    Token loopVar4 = current(); state.expect(kIdentifier, "...", diagnostics)
    if (loopVar4.text != loopVar.text) → diagnostic
    // expect +
    state.match(kUnknown) // consume '+'
    Token stepExpr = current(); state.expect(kExpression, "Ожидался шаг цикла", diagnostics)
    state.expect(kLeftBrace, "Ожидался символ { после заголовка for", diagnostics)
    // Parse body: sequence of layer declarations
    while (current().type != kRightBrace && current().type != kEof) {
      skipNewlines(state)
      if (current().type == kKeywordLayer) {
        state.match(kKeywordLayer)
        parseLayer(state, program, diagnostics) → add to forLoop.body
      } else {
        break
      }
    }
    state.expect(kRightBrace, "Ожидался символ } после тела for", diagnostics)
    ```

    **Loop bound validation:** After parsing, validate startExpression, endExpression, stepExpression are compile-time integer constants. If not: diagnostic "Границы цикла for должны быть целыми числами".

    **Modify `parseProgram()`:** Add handling for `kKeywordFor`:
    ```
    if (state.current().type == kKeywordFor) {
      state.match(kKeywordFor);
      parseForLoop(state, parsedProgram, diagnostics);
      continue;
    }
    ```

    **All error messages in Russian**, following existing pattern (e.g., "Ожидалось имя sprite", "Неожиданный токен в layer").
  </action>
  <verify>
    <automated>cd /home/ros2/mylamp && platformio test -e native --filter "test_dsl_parser" -v 2>&1 | tail -30</automated>
  </verify>
  <acceptance_criteria>
    - `parseSprite()` handles multi-frame sprite: `sprite.frames.size()` > 0 after parsing
    - `parseSprite()` backward compat: existing single-bitmap sprites produce `sprite.frames.empty() && !sprite.bitmap.empty()`
    - `parseLayer()` sets `layer.frameExpression` when `frame = ...` present in layer
    - `parseForLoop()` populates `program.forLoops` with ForLoopStatement entries
    - For loop with 3 iterations and 1 layer body: `forLoop.body.size() == 1`
    - Malformed for-loop produces Russian diagnostic containing "цикла"
    - All existing parser tests pass unchanged (D-04)
    - New test: `test_parser_parses_sprite_with_multiple_frames` passes
    - New test: `test_parser_parses_for_loop_with_layers` passes
    - New test: `test_parser_parses_layer_with_frame_expression` passes
  </acceptance_criteria>
</task>

</tasks>

<threat_model>
## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| DSL source string → Lexer | Untrusted user-provided text; must handle arbitrary input without crash |
| Lexer output → Parser | Token stream from potentially oversized source; parser must validate token sequences |

## STRIDE Threat Register

| Threat ID | Category | Component | Disposition | Mitigation Plan |
|-----------|----------|-----------|-------------|-----------------|
| T-5-01 | Denial of Service | Lexer::tokenize() | mitigate | Reject source strings exceeding 64KB before tokenization; existing `std::string` size check at call site in LiveProgramService |
| T-5-02 | Tampering | Parser::parseForLoop() | mitigate | Validate loop bounds are integer literals at parse time; reject non-constant expressions with Russian diagnostic; validate loop var name consistency across for-header clauses |
| T-5-03 | Information Disclosure | Lexer/Parser diagnostics | accept | Diagnostics contain only Russian text and source line numbers; no filesystem paths, stack traces, or internal state leaked — consistent with existing pattern |

</threat_model>

<verification>
- [ ] `platformio test -e native --filter "test_dsl_parser" -v` passes all existing + new tests
- [ ] Existing .lux effects from snippets.ts parse without new diagnostics
- [ ] `grep -c "kKeywordFor" include/live/dsl/Token.h` > 0
- [ ] `grep -c "SpriteFrameDeclaration" include/live/dsl/Ast.h` > 0
</verification>

<success_criteria>
- Token.h has 3 new token types (kKeywordFor, kKeywordFrame, kSemicolon)
- Ast.h has SpriteFrameDeclaration, ForLoopStatement, frameExpression on LayerDeclaration
- Lexer tokenizes multi-frame sprites and for-loops without crashing on any input
- Parser builds correct AST for all new constructs
- All existing parser tests pass (D-04 backward compat)
- New parser tests for multi-frame sprite, for-loop, layer frame expression pass
</success_criteria>

<output>
After completion, create `.planning/phases/05-dsl/05-01-SUMMARY.md`
</output>

---

## Plan 05-02: Compiler + Executor — For-loop Unrolling, Frame Selection

---
phase: 05-dsl
plan: 02
type: execute
wave: 2
depends_on: [05-01]
files_modified:
  - include/live/runtime/CompiledProgram.h
  - src/live/runtime/Compiler.cpp
  - src/live/runtime/Executor.cpp
  - test/test_dsl_executor/test_main.cpp
autonomous: true
requirements: [DSL-01, DSL-02]

must_haves:
  truths:
    - "Compiler unrolls for-loops into multiple CompiledLayer instances"
    - "Compiler compiles sprite frames into CompiledSprite.frames vector"
    - "Executor selects sprite frame by evaluating frame expression modulo frame count"
    - "Executor renders correct frame pixels based on evaluated frame index"
    - "MAX_UNROLLED_LAYERS=64 enforced at compile time"
    - "Backward compat: old single-frame sprites render identically (D-04)"
  artifacts:
    - path: "include/live/runtime/CompiledProgram.h"
      provides: "CompiledSprite.frames, CompiledLayer.frameExpression, ExpressionOp::kLoopIndex"
      contains: "std::vector<std::vector<CompiledSpritePixel>> frames"
    - path: "src/live/runtime/Compiler.cpp"
      provides: "compileSpriteFrames(), unrollForLoops(), frame expression compilation"
      contains: "MAX_UNROLLED_LAYERS"
    - path: "src/live/runtime/Executor.cpp"
      provides: "Frame index evaluation, multi-frame sprite pixel selection"
      contains: "frames\\[frameIndex\\]"
    - path: "test/test_dsl_executor/test_main.cpp"
      provides: "Tests for frame selection, for-loop unrolling, backward compat"
      contains: "test_executor_renders_correct_sprite_frame"
  key_links:
    - from: "Compiler.cpp unrollForLoops()"
      to: "Executor.cpp render()"
      via: "CompiledLayer instances emitted by unrolling, rendered by executor loop"
      pattern: "compiled\\.layers\\.push_back"
    - from: "Executor.cpp frame selection"
      to: "CompiledSprite.frames"
      via: "evaluateNode(frameExpression) % frames.size()"
      pattern: "frames\\[.*frameIndex"
</objective>

<execution_context>
@~/.copilot/get-shit-done/workflows/execute-plan.md
@~/.copilot/get-shit-done/templates/summary.md
</execution_context>

<context>
@.planning/phase-5-dsl/05-CONTEXT.md
@.planning/phase-5-dsl/05-RESEARCH.md
@.planning/phase-5-dsl/05-VALIDATION.md
@include/live/runtime/CompiledProgram.h
@src/live/runtime/Compiler.cpp
@src/live/runtime/Executor.cpp
@test/test_dsl_executor/test_main.cpp

<interfaces>
From include/live/runtime/CompiledProgram.h (existing):
```cpp
enum class ExpressionOp { kConstant, kTime, kDeltaTime, kTemperature, kHumidity,
  kCoordX, kCoordY, kCoordNx, kCoordNy, kAdd, kSubtract, kMultiply, kDivide,
  kModulo, kNegate, kSin, kCos, kAbs, kMin, kMax, kClamp, kMix, kSmoothstep };

struct CompiledSpritePixel { int16_t x, y; };
struct CompiledSprite { std::string name; int16_t width, height; std::vector<CompiledSpritePixel> pixels; };
struct CompiledLayer { std::string name; uint16_t spriteIndex; CompiledColor color;
  int16_t xExpression, yExpression, scaleExpression, rotationExpression; BlendMode blendMode; int16_t visibleExpression; };
struct CompiledProgram { std::string effectName; std::vector<CompiledSprite> sprites;
  std::vector<ExpressionNode> expressions; std::vector<CompiledLayer> layers; };
```

Key patterns from Compiler.cpp:
- `compileSprite()` iterates bitmap chars, builds `CompiledSpritePixel` vector for `#` chars
- `compileText()` uses `PixelFont.h` glyph lookup to generate pixels
- `Compiler::compile()` iterates program.sprites → compileSprite, program.texts → compileText, program.layers → compiledLayer
- Expression compilation via `ExpressionCompiler` class with `compile(expr, rootIndex, diagnostics, sourceLine)`
- Color compilation via `compileColor()` which handles `rgb(...)` and `hsv(...)` with 3-channel argument splitting

Key patterns from Executor.cpp:
- `render()` clears FrameBuffer, iterates `program.layers`, evaluates visibility/x/y/scale/rotation, then iterates `sprite.pixels` with scale and rotation transforms
- `renderSpritePixel()` handles XY-swap (DSL x→physical Y, DSL y→physical X) and color blending
- `evaluateNode()` recursively evaluates ExpressionNode tree with depth limit `kMaxExpressionDepth=64`
- Frame selection must be done BEFORE the pixel iteration loop: evaluate `layer.frameExpression`, compute `frameIndex = int(eval) % sprite.frames.size()`, then iterate `sprite.frames[frameIndex]` instead of `sprite.pixels`

From AppConfig.h: `kMaxExpressionDepth = 64`
</interfaces>
</context>

<tasks>

<task type="auto">
  <name>Task 1: Extend CompiledProgram.h — frames vector, frameExpression, kLoopIndex</name>
  <files>include/live/runtime/CompiledProgram.h</files>
  <read_first>include/live/runtime/CompiledProgram.h</read_first>
  <action>
    **CompiledSprite changes (per D-01, D-04):**
    Add field `std::vector<std::vector<CompiledSpritePixel>> frames` — each inner vector is one frame's pixels.
    Keep existing `std::vector<CompiledSpritePixel> pixels` — used when frames is empty (single-frame backward compat, D-04).
    Invariant: if `frames` is non-empty, `pixels` is a copy of `frames[0]` for backward-compatible rendering when no frame selection is done. If `frames` is empty, `pixels` is the only data.

    **CompiledLayer changes (per D-02):**
    Add field `int16_t frameExpression = -1` — expression index for frame selection; -1 means "use frame 0" (default, backward compat D-04).

    **ExpressionOp changes (per D-03):**
    Add `kLoopIndex` to `enum class ExpressionOp` — represents the compile-time loop variable value. During for-loop unrolling, this op is replaced with `kConstant` at the concrete iteration value. Never appears in final compiled output.

    **MAX_UNROLLED_LAYERS constant:**
    Add `static constexpr int16_t kMaxUnrolledLayers = 64` inside `namespace lamp::live::runtime` — hard cap on total layers after for-loop unrolling, per RESEARCH.md safety recommendation.
  </action>
  <verify>
    <automated>grep -c "frames" include/live/runtime/CompiledProgram.h | grep -v '^#' && grep -c "frameExpression" include/live/runtime/CompiledProgram.h | grep -v '^#' && grep -c "kLoopIndex" include/live/runtime/CompiledProgram.h | grep -v '^#' && grep -c "kMaxUnrolledLayers" include/live/runtime/CompiledProgram.h | grep -v '^#'</automated>
  </verify>
  <acceptance_criteria>
    - `CompiledSprite` has `std::vector<std::vector<CompiledSpritePixel>> frames` field
    - `CompiledLayer` has `int16_t frameExpression = -1` field
    - `ExpressionOp` enum has `kLoopIndex` entry
    - `kMaxUnrolledLayers = 64` constant defined
    - `grep -c "kMaxUnrolledLayers" include/live/runtime/CompiledProgram.h` > 0
    - `grep -c "kLoopIndex" include/live/runtime/CompiledProgram.h` > 0
  </acceptance_criteria>
</task>

<task type="auto">
  <name>Task 2: Extend Compiler — compileSpriteFrames, unrollForLoops, frame expression</name>
  <files>src/live/runtime/Compiler.cpp</files>
  <read_first>src/live/runtime/Compiler.cpp, include/live/runtime/CompiledProgram.h, include/live/dsl/Ast.h</read_first>
  <action>
    **Modify `compileSprite()` (per D-01):**
    Add overload or modify existing to accept `const dsl::SpriteDeclaration&` and handle both cases:
    - If `sprite.frames` is non-empty: iterate each `SpriteFrameDeclaration`, call existing bitmap→pixels logic per frame, push result vector to `compiled.frames`. Set `compiled.pixels = compiled.frames[0]` (copy of frame 0 for backward compat).
    - If `sprite.frames` is empty: existing single-bitmap logic (D-04).

    **Modify `Compiler::compile()` (per D-03):**
    After compiling sprites and texts, BEFORE compiling layers, insert for-loop unrolling logic:
    ```
    for (const dsl::ForLoopStatement& forLoop : program.forLoops) {
      int startVal = std::stoi(forLoop.startExpression);
      int endVal = std::stoi(forLoop.endExpression);
      int stepVal = std::stoi(forLoop.stepExpression);
      std::string compOp = forLoop.comparisonOperator;
      int iterationCount = 0;
      for (int i = startVal; evaluateComparison(i, endVal, compOp); i += stepVal) {
        iterationCount++;
      }
      // Check before unrolling
      if (compiled.layers.size() + iterationCount * forLoop.body.size() > kMaxUnrolledLayers) {
        diagnostics.push_back(makeDiagnostic(0,
          "Слишком много слоёв после развёртки for. Максимум: " + std::to_string(kMaxUnrolledLayers)));
        return false;
      }
      // Unroll
      for (int i = startVal; evaluateComparison(i, endVal, compOp); i += stepVal) {
        std::string iStr = std::to_string(i);
        for (dsl::LayerDeclaration layerTemplate : forLoop.body) {
          layerTemplate.name += "_" + iStr;
          replaceAll(layerTemplate.xExpression, forLoop.loopVariable, iStr);
          replaceAll(layerTemplate.yExpression, forLoop.loopVariable, iStr);
          replaceAll(layerTemplate.colorExpression, forLoop.loopVariable, iStr);
          replaceAll(layerTemplate.scaleExpression, forLoop.loopVariable, iStr);
          replaceAll(layerTemplate.rotationExpression, forLoop.loopVariable, iStr);
          replaceAll(layerTemplate.visibleExpression, forLoop.loopVariable, iStr);
          replaceAll(layerTemplate.frameExpression, forLoop.loopVariable, iStr);
          // Compile the substituted layer normally
          compileLayer(layerTemplate, compiled, diagnostics);  // inline the existing layer compilation
        }
      }
    }
    ```
    The `replaceAll()` function does simple string substitution of loop variable name with the integer literal. This is string-level substitution before expression compilation — simple and sufficient for integer loop vars.

    **Frame expression compilation (per D-02):**
    In the per-layer compilation loop (existing), add after other expression compilations:
    ```
    if (!layer.frameExpression.empty()) {
      if (!expressionCompiler.compile(layer.frameExpression, compiledLayer.frameExpression,
                                      diagnostics, layer.frameLine)) {
        return false;
      }
    }
    ```

    **Helper: `evaluateComparison(int val, int bound, const std::string& op)`:**
    - `"<"` → `val < bound`
    - `"<="` → `val <= bound`
    - `">"` → `val > bound`
    - `">="` → `val >= bound`

    **Helper: `replaceAll(std::string& str, const std::string& from, const std::string& to)`:**
    Simple find-and-replace loop. Since loop variable names are short identifiers and replacement is integer strings, no regex needed.
  </action>
  <verify>
    <automated>cd /home/ros2/mylamp && platformio test -e native --filter "test_dsl_executor" -v 2>&1 | tail -30</automated>
  </verify>
  <acceptance_criteria>
    - `compileSprite()` produces `CompiledSprite.frames` from multi-frame `SpriteDeclaration`
    - Single-frame sprites still produce `CompiledSprite.pixels` only (D-04)
    - For-loop with `i = 0; i < 3; i = i + 1` and 1 body layer unrolls to 3 `CompiledLayer` entries
    - Loop variable `i` substituted with `"0"`, `"1"`, `"2"` in all expressions
    - `MAX_UNROLLED_LAYERS=64` enforced: exceeding it produces Russian diagnostic "Слишком много слоёв после развёртки for"
    - Frame expression compiled: `layer.frameExpression` non-empty → `compiledLayer.frameExpression` is valid expression index
    - `grep -c "unroll\|MAX_UNROLLED" src/live/runtime/Compiler.cpp` > 0
    - `grep -c "frameExpression" src/live/runtime/Compiler.cpp` > 0
  </acceptance_criteria>
</task>

<task type="auto">
  <name>Task 3: Extend Executor — frame index evaluation, multi-frame sprite rendering</name>
  <files>src/live/runtime/Executor.cpp</files>
  <read_first>src/live/runtime/Executor.cpp, include/live/runtime/CompiledProgram.h</read_first>
  <action>
    **Frame selection in render() (per D-02):**
    In the layer rendering loop, BEFORE the pixel iteration, add frame index evaluation:
    ```
    const CompiledSprite& sprite = program.sprites[layer.spriteIndex];

    // Determine which frame's pixels to render
    const std::vector<CompiledSpritePixel>* framePixels = &sprite.pixels;  // default: single-frame
    if (!sprite.frames.empty()) {
      int frameIndex = 0;  // default frame 0 per D-04
      if (layer.frameExpression >= 0) {
        float rawIndex = evaluateNode(program.expressions, layer.frameExpression, baseContext);
        frameIndex = static_cast<int>(std::floor(rawIndex)) % static_cast<int>(sprite.frames.size());
        if (frameIndex < 0) frameIndex += static_cast<int>(sprite.frames.size());
      }
      framePixels = &sprite.frames[static_cast<size_t>(frameIndex)];
    }

    // Use framePixels instead of sprite.pixels in the pixel iteration loop below
    for (const CompiledSpritePixel& pixel : *framePixels) {
      // ... existing scale/rotation/rendering logic ...
    }
    ```

    The rest of the rendering loop (scale, rotation, renderSpritePixel) remains unchanged. The key change is selecting which pixel vector to iterate based on the evaluated frame index.

    **Backward compatibility (per D-04):**
    - If `sprite.frames.empty()` → use `sprite.pixels` (existing behavior)
    - If `sprite.frames` non-empty but `layer.frameExpression == -1` → use `frames[0]` (default frame)
    - If `sprite.frames` non-empty and `layer.frameExpression >= 0` → evaluate expression, compute index, use `frames[index]`

    **For-loop unrolled layers:** No executor changes needed — unrolled layers are just regular `CompiledLayer` instances with substituted expression strings. The executor renders them identically to hand-written layers.
  </action>
  <verify>
    <automated>cd /home/ros2/mylamp && platformio test -e native --filter "test_dsl_executor" -v 2>&1 | tail -30</automated>
  </verify>
  <acceptance_criteria>
    - Single-frame sprites render identically to pre-change behavior (D-04)
    - Multi-frame sprite: `frame = 0` renders frame 0 pixels
    - Multi-frame sprite: `frame = 1` renders frame 1 pixels
    - Multi-frame sprite: `frame = (t * 4) % 2` cycles between frame 0 and 1 as t increases
    - Frame index bounded: `frame = 99` on 2-frame sprite → renders frame 1 (99 % 2 = 1)
    - Negative frame index handled: `frame = -1` on 2-frame sprite → renders frame 1 (-1+2=1)
    - Unrolled for-loop layers render correctly (each layer instance at its substituted position)
    - `grep -c "frames\[" src/live/runtime/Executor.cpp` > 0
    - `grep -c "frameExpression" src/live/runtime/Executor.cpp` > 0
  </acceptance_criteria>
</task>

<task type="auto" tdd="true">
  <name>Task 4: Write executor tests for frame selection and for-loop unrolling</name>
  <files>test/test_dsl_executor/test_main.cpp</files>
  <read_first>test/test_dsl_executor/test_main.cpp, src/live/runtime/Executor.cpp</read_first>
  <behavior>
    - Test 1: `test_executor_renders_correct_sprite_frame` — 2-frame sprite, frame=0 → pixel at (x0,y0); frame=1 → pixel at (x1,y1)
    - Test 2: `test_executor_frame_expression_modulo` — 3-frame sprite, frame=(t*4)%3 expression → verify cycling through frames
    - Test 3: `test_executor_for_loop_unrolls_layers` — for i=0;i<3;i=i+1 with layer using `x = i*5` → verify 3 distinct x positions rendered
    - Test 4: `test_executor_single_frame_sprite_still_works` — old-style single-bitmap sprite renders correctly (backward compat D-04)
    - Test 5: `test_executor_frame_index_bounded` — frame=99 on 2-frame sprite → renders frame 1
    - Test 6: `test_executor_max_unrolled_layers_enforced` — for-loop exceeding 64 layers → compile fails with diagnostic
  </behavior>
  <action>
    Add 6 new test functions to `test/test_dsl_executor/test_main.cpp` following existing patterns:
    - Use `compileSource()` helper (already defined in test file)
    - Use `lamp::MatrixLayout` + `lamp::FrameBuffer` for rendering
    - Assert pixel values via `frameBuffer.getPixel(x, y)`
    - Follow existing test naming: `test_executor_*`

    For test 6 (MAX_UNROLLED_LAYERS enforcement), create a DSL source with a for-loop that would produce 65+ layers and verify compilation returns false with a diagnostic.

    Include the new tests in the test runner (`RUN_TEST` macros) at the bottom of the file.
  </action>
  <verify>
    <automated>cd /home/ros2/mylamp && platformio test -e native --filter "test_dsl_executor" -v</automated>
  </verify>
  <acceptance_criteria>
    - All 6 new tests pass
    - All existing executor tests pass unchanged (backward compat)
    - `grep -c "test_executor_renders_correct_sprite_frame\|test_executor_frame_expression_modulo\|test_executor_for_loop_unrolls_layers\|test_executor_single_frame_sprite_still_works\|test_executor_frame_index_bounded\|test_executor_max_unrolled_layers_enforced" test/test_dsl_executor/test_main.cpp` == 6
  </acceptance_criteria>
</task>

</tasks>

<threat_model>
## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| DSL source → Compiler | AST from parser; compiler must validate before emitting bytecode |
| CompiledProgram → Executor | Bytecode consumed at render time; must be safe against malformed indices |
| Expression evaluation → FrameBuffer | Per-pixel evaluation must not overflow framebuffer bounds |

## STRIDE Threat Register

| Threat ID | Category | Component | Disposition | Mitigation Plan |
|-----------|----------|-----------|-------------|-----------------|
| T-5-04 | Denial of Service | Compiler::compile() for-loop unrolling | mitigate | Enforce `kMaxUnrolledLayers=64`; reject programs exceeding limit with Russian diagnostic before any memory allocation |
| T-5-05 | Tampering | Executor::render() frame index | mitigate | Frame index bounded by `% frames.size()`; negative indices wrapped; null/empty frames vector falls back to sprite.pixels |
| T-5-06 | Elevation of Privilege | ExpressionCompiler | accept | DSL expressions are sandboxed (no eval, no system calls, no file access); expression grammar is fixed and finite — same security posture as existing DSL |

</threat_model>

<verification>
- [ ] `platformio test -e native --filter "test_dsl_executor" -v` all tests pass
- [ ] `platformio test -e native -v` full suite passes (no regressions)
- [ ] Multi-frame sprite renders correct frame on FrameBuffer
- [ ] For-loop unrolls to correct number of layers
- [ ] Backward compat: existing .lux effects compile and execute identically
</verification>

<success_criteria>
- CompiledSprite supports multiple frames via `frames` vector
- Compiler unrolls for-loops into CompiledLayer instances with string substitution
- MAX_UNROLLED_LAYERS=64 enforced with Russian diagnostic
- Executor selects correct frame by evaluating frame expression modulo frame count
- All existing DSL effects compile and render identically (D-04)
- 6 new executor tests pass
- Full test suite passes
</success_criteria>

<output>
After completion, create `.planning/phases/05-dsl/05-02-SUMMARY.md`
</output>

---

## Plan 05-03: Demo Effects + Factory Presets

---
phase: 05-dsl
plan: 03
type: execute
wave: 3
depends_on: [05-02]
files_modified:
  - resources/demo/*.lux (8 new files)
  - src/main.cpp
autonomous: true
requirements: [DSL-01, DSL-03]

must_haves:
  truths:
    - "All 8 demo effects are valid Lux DSL that compiles and renders on the cylinder"
    - "Nyan Cat shows a running cat with rainbow trail"
    - "Mario shows 8-bit Mario walking around the cylinder"
    - "Plasma shows fullscreen per-pixel sin/cos abstraction"
    - "Scrolling Text shows text moving along the cylinder circumference"
    - "Snake shows a snake game on the cylinder"
    - "Fire shows particles rising upward"
    - "Starfield shows flying stars"
    - "DNA shows a rotating double helix"
    - "Factory presets seeded on first boot when /presets/ directory is empty"
    - "Factory presets appear in web UI preset list after first boot"
  artifacts:
    - path: "resources/demo/nyan-cat.lux"
      provides: "Nyan Cat effect source"
      min_lines: 20
    - path: "resources/demo/mario.lux"
      provides: "Mario effect source"
      min_lines: 20
    - path: "resources/demo/plasma.lux"
      provides: "Plasma/Perlin effect source"
      min_lines: 10
    - path: "resources/demo/scrolling-text.lux"
      provides: "Scrolling Text effect source"
      min_lines: 10
    - path: "resources/demo/snake.lux"
      provides: "Snake effect source"
      min_lines: 15
    - path: "resources/demo/fire-particles.lux"
      provides: "Fire particles effect source"
      min_lines: 10
    - path: "resources/demo/starfield.lux"
      provides: "Starfield effect source"
      min_lines: 10
    - path: "resources/demo/dna.lux"
      provides: "DNA spiral effect source"
      min_lines: 10
    - path: "src/main.cpp"
      provides: "Factory preset seeding logic"
      contains: "seedFactoryPresets"
  key_links:
    - from: "resources/demo/*.lux"
      to: "PresetRepository::save()"
      via: "seedFactoryPresets() reads .lux files, creates PresetModel, calls save()"
      pattern: "g_presetRepository\\.save"
    - from: "seedFactoryPresets()"
      to: "main.cpp setup()"
      via: "Called during initializeFileSystem() if /presets/ is empty"
      pattern: "seedFactoryPresets"
</objective>

<execution_context>
@~/.copilot/get-shit-done/workflows/execute-plan.md
@~/.copilot/get-shit-done/templates/summary.md
</execution_context>

<context>
@.planning/phase-5-dsl/05-CONTEXT.md
@.planning/phase-5-dsl/05-RESEARCH.md
@.planning/phase-5-dsl/05-UI-SPEC.md
@src/main.cpp
@include/live/PresetRepository.h
@include/live/PresetModel.h
@include/storage/ContentPaths.h
@docs/DSL.md

<interfaces>
From include/live/PresetRepository.h:
```cpp
class PresetRepository {
  bool save(const PresetModel& preset);
  std::vector<PresetModel> list() const;
};
```

From include/live/PresetModel.h:
```cpp
struct PresetModel {
  std::string id, name, source, createdAt, updatedAt;
  std::vector<std::string> tags;
  PresetOptions options;
};
```

From include/storage/ContentPaths.h:
```cpp
static constexpr char kPresetsDirectory[] = "/presets";
```

From src/main.cpp — existing boot sequence:
- `initializeFileSystem()` mounts LittleFS, calls `ensureContentDirectories()`, lists presets/playlists
- `g_presetRepository` is a global `PresetRepository` instance
- Factory seeding should be inserted after `g_fileStore.setReady(true)` and before the preset list log
</interfaces>
</context>

<tasks>

<task type="auto">
  <name>Task 1: Author 8 demo .lux effect files</name>
  <files>resources/demo/nyan-cat.lux, resources/demo/mario.lux, resources/demo/plasma.lux, resources/demo/scrolling-text.lux, resources/demo/snake.lux, resources/demo/fire-particles.lux, resources/demo/starfield.lux, resources/demo/dna.lux</files>
  <read_first>docs/DSL.md, frontend/src/editor/snippets.ts, .planning/phase-5-dsl/05-CONTEXT.md</read_first>
  <action>
    Create 8 `.lux` effect files in `resources/demo/`. Each must be valid Lux DSL that compiles and renders on the 32×16 cylindrical matrix. Use new `frame` and `for` constructs where appropriate (per D-01, D-02, D-03, D-08).

    **Effect design guidelines (per D-06, D-07):**

    1. **nyan-cat.lux** — Nyan Cat with rainbow trail:
       - Multi-frame sprite: cat body with 4 walk frames (paws alternate), each ~8×6 pixels
       - Rainbow trail: fullscreen layer with per-pixel color using `smoothstep` on distance from cat x position, cycling hue with `t`. Use `hsv(hue + nx*30, 1, 1)` pattern.
       - Cat `x = (t * 8) % 32`, `y = 8`
       - `frame = (t * 4) % 4` for walk animation

    2. **mario.lux** — 8-bit Mario walking around cylinder:
       - Multi-frame sprite: Mario with 3 walk frames, each ~6×8 pixels
       - `x = (t * 6) % 32`, `y = 4`, `frame = (t * 6) % 3`

    3. **plasma.lux** — Fullscreen per-pixel abstract:
       - Single fullscreen sprite (32×16 all `#`)
       - Color expression: `hsv(sin(nx*8+t)*30 + cos(ny*6+t*0.7)*30 + t*20, 1, 0.5+0.5*sin(nx*5+ny*4+t))` — layered sin/cos waves
       - No for-loop needed; pure per-pixel math

    4. **scrolling-text.lux** — Text moving around cylinder:
       - `text msg "ПРИВЕТ МОЯ ЛАМПА "` (trailing space for wrap)
       - for-loop approach: `for i = 0; i < 18; i = i + 1 { layer char{i} { use msg; x = (t*5 + i*4) % 32; y = 2 } }` — each layer renders same text sprite at different offset
       - OR simpler: single layer with `x = (t * 6) % 32` and use text sprite that naturally wraps

    5. **snake.lux** — Snake game simulation:
       - 1×1 dot sprite
       - for-loop: `for i = 0; i < 10; i = i + 1 { layer seg{i} { use dot; x = (sin(t*2 + i*0.6)*12 + 16); y = (cos(t*2 + i*0.6)*6 + 8); color hsv(i*30, 1, 1); scale = 1 } }` — wave of dots simulating snake body

    6. **fire-particles.lux** — Fire rising:
       - 1×1 dot sprite
       - for-loop: `for i = 0; i < 12; i = i + 1 { layer p{i} { use dot; x = (sin(i*2.7 + t)*3 + 8); y = (t*3 + i*2) % 32; color hsv(15 + i*2, 1, 1 - i*0.08); scale = 1 + (i%2) } }` — staggered particles rising at different speeds

    7. **starfield.lux** — Flying stars:
       - 1×1 dot sprite
       - for-loop: `for i = 0; i < 20; i = i + 1 { layer s{i} { use dot; x = ((i*7 + 3) % 16); y = (t*(3+i%4) + i*11) % 32; color rgb(200+55*sin(t+i), 200+55*cos(t+i*0.7), 255); scale = 1; blend = add } }` — stars at different speeds with twinkling

    8. **dna.lux** — DNA double helix:
       - 1×1 dot sprite
       - Two for-loops (or one with doubled body): `for i = 0; i < 16; i = i + 1 { layer h1{i} { use dot; x = (8 + sin(t*2 + i*0.4)*6); y = i*2; color rgb(80, 200, 255); scale = 1 }; layer h2{i} { use dot; x = (8 + sin(t*2 + i*0.4 + 3.1415)*6); y = i*2; color rgb(255, 80, 120); scale = 1 } }` — two offset sine waves

    **Testing each effect:** After each .lux file is written, validate by running:
    - Embed the file content into a test that calls `parseProgram()` + `Compiler::compile()` — verify no diagnostics
    - Or manually: paste into editor, click "Проверить", verify no errors

    **Pixel art for sprites:** Adapt Nyan Cat and Mario pixel art for 32×16 cylinder. Nyan Cat body ~8×6 pixels (pop-tart shape), Mario ~6×8 pixels. Use `#` for lit pixels, `.` for transparent. Reference: https://encrypted-tbn0.gstatic.com/images?q=tbn:ANd9GcQ0M95o199E58oG5w2NrKrSRRpgZ3sUecLCuS74SVf44QhCL3MxtTg52nYIOYFEYgFs4lvwmPAJsuXdj8LUCQ5EacE0xutJ3rB_kXVqHS45Mg
  </action>
  <verify>
    <automated>cd /home/ros2/mylamp && for f in resources/demo/*.lux; do echo "=== $f ===" && cat "$f"; done | head -200</automated>
  </verify>
  <acceptance_criteria>
    - 8 `.lux` files exist in `resources/demo/`
    - Each file starts with `effect "..."` header
    - Each file parses without diagnostics when run through `parseProgram()`
    - Each file compiles without diagnostics when run through `Compiler::compile()`
    - Nyan Cat uses multi-frame sprite (≥2 frames) and `frame = ...` expression
    - Mario uses multi-frame sprite (≥2 frames) and `frame = ...` expression
    - At least 3 effects use `for` loops
    - Snake uses `for` loop for body segments
    - All effects render on 32×16 FrameBuffer without pixel out-of-bounds
    - `grep -c 'effect "' resources/demo/*.lux | grep -v ':0$'` shows 8 matches
    - `grep -c 'frame =' resources/demo/*.lux` ≥ 2 (Nyan Cat + Mario)
    - `grep -c '\bfor\b' resources/demo/*.lux` ≥ 3
  </acceptance_criteria>
</task>

<task type="auto">
  <name>Task 2: Seed factory presets on first boot in main.cpp</name>
  <files>src/main.cpp</files>
  <read_first>src/main.cpp, include/live/PresetRepository.h, include/live/PresetModel.h, include/storage/ContentPaths.h</read_first>
  <action>
    **Factory preset seeding logic (per D-09):**
    Add a function `seedFactoryPresets()` in `src/main.cpp` (or a new file `src/live/FactoryPresets.cpp` — discretion; `main.cpp` is simpler):

    ```
    void seedFactoryPresets() {
      if (!g_fileStore.isReady()) return;
      const std::vector<std::string> existing = g_fileStore.list(lamp::storage::kPresetsDirectory);
      if (!existing.empty()) return;  // already seeded

      struct DemoEntry { const char* id; const char* name; const char* filename; };
      const DemoEntry demos[] = {
        {"nyan-cat", "Нян Кот", "nyan-cat.lux"},
        {"mario", "Марио", "mario.lux"},
        {"plasma", "Плазма", "plasma.lux"},
        {"scrolling-text", "Бегущая строка", "scrolling-text.lux"},
        {"snake", "Змейка", "snake.lux"},
        {"fire-particles", "Огоньки", "fire-particles.lux"},
        {"starfield", "Звёздное поле", "starfield.lux"},
        {"dna", "Спираль ДНК", "dna.lux"},
      };

      for (const DemoEntry& demo : demos) {
        // Read .lux file from embedded resources (LittleFS or compile-time embedding)
        // Since these are factory presets, embed them as string constants
        // OR: store .lux files in LittleFS data directory and read them
        // Simplest: embed as string constants using the same pattern as embedded_resources.h
        // But for now, since files are in resources/demo/, they need to be embedded at build time
        // Alternative: read from a known LittleFS path pre-populated by the build system

        // For Phase 5, use compile-time string embedding:
        // The .lux files are compiled into the firmware via embed_resources.py or manual #include
        // Check if a resource embedding mechanism exists, otherwise use raw string literals
      }
    }
    ```

    **Integration into boot sequence:**
    Call `seedFactoryPresets()` in `initializeFileSystem()`, AFTER `g_fileStore.setReady(true)` and AFTER the existing preset list log, but before any network-dependent operations:

    In `initializeFileSystem()`:
    ```cpp
    g_fileStore.setReady(true);

    const std::vector<std::string> presets = g_fileStore.list(lamp::storage::kPresetsDirectory);
    // ... existing log ...

    // Seed factory presets if /presets/ is empty (first boot)
    if (presets.empty()) {
      seedFactoryPresets();
    }
    ```

    **Embedding strategy:** Since the ESP32 build system uses `embed_resources.py` for web resources and there's already an `embedded_resources.h` pattern, the simplest approach is:
    - Create a header `resources/demo/factory_presets.h` (auto-generated or hand-written) with `const char*` string constants for each .lux file
    - Include it in `main.cpp`
    - In `seedFactoryPresets()`, create `PresetModel` for each, call `g_presetRepository.save(preset)`

    Alternatively (simpler for Phase 5): embed the .lux source strings directly as `static const char*` in `seedFactoryPresets()` function body. This avoids build system changes.

    Use the simpler inline approach: define each demo source as a `static const char*` in the function body, create `PresetModel` with `id`, `name`, `source`, `createdAt`/`updatedAt` set to current ISO timestamp, empty `tags`, default `options`, and call `g_presetRepository.save(model)`.

    Log each seeded preset: `Serial.printf("factory: seeded preset %s\n", demo.id);`
  </action>
  <verify>
    <automated>grep -c "seedFactoryPresets" src/main.cpp | grep -v '^#'</automated>
  </verify>
  <acceptance_criteria>
    - `seedFactoryPresets()` function exists in `src/main.cpp`
    - Function checks `g_fileStore.list("/presets/")` — skips if non-empty
    - Function creates 8 `PresetModel` entries with correct ids: nyan-cat, mario, plasma, scrolling-text, snake, fire-particles, starfield, dna
    - Each preset has Russian `name` matching UI-SPEC snippet names
    - Each preset has `source` field with valid Lux DSL content
    - Function calls `g_presetRepository.save()` for each
    - Called from `initializeFileSystem()` after `g_fileStore.setReady(true)` when presets list is empty
    - `grep -c "seedFactoryPresets" src/main.cpp` > 0
    - `grep -c "g_presetRepository.save" src/main.cpp` ≥ 8 (inside seed function)
  </acceptance_criteria>
</task>

</tasks>

<threat_model>
## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| Embedded .lux source → PresetRepository | Factory preset source strings are compile-time constants; no user input |
| LittleFS write → flash wear | 8 writes on first boot only; negligible wear |

## STRIDE Threat Register

| Threat ID | Category | Component | Disposition | Mitigation Plan |
|-----------|----------|-----------|-------------|-----------------|
| T-5-07 | Denial of Service | seedFactoryPresets() | accept | 8 small preset writes on first boot only; each < 2KB; total ~16KB — negligible flash and time cost |
| T-5-08 | Tampering | Factory preset content | accept | Preset source is compile-time constant embedded in firmware; tampering requires re-flashing firmware — same trust model as rest of firmware |

</threat_model>

<verification>
- [ ] All 8 `.lux` files parse and compile without errors
- [ ] `seedFactoryPresets()` present and wired in `initializeFileSystem()`
- [ ] Flash dev build, check serial output for "factory: seeded preset" messages
- [ ] After first boot, web UI preset list shows 8 demo presets with Russian names
</verification>

<success_criteria>
- 8 demo .lux effect files created, all valid and compilable
- Nyan Cat and Mario use multi-frame sprite animation
- At least 3 effects use for-loops
- Factory presets seeded automatically on first boot when /presets/ empty
- Demos appear in web UI preset list
</success_criteria>

<output>
After completion, create `.planning/phases/05-dsl/05-03-SUMMARY.md`
</output>

---

## Plan 05-04: Frontend — Share, Save Fix, Snippets, Help, Highlighting

---
phase: 05-dsl
plan: 04
type: execute
wave: 4
depends_on: [05-03]
files_modified:
  - frontend/src/main.ts
  - frontend/src/ui/shellTemplate.ts
  - frontend/src/editor/snippets.ts
  - frontend/src/editor/help.ts
  - frontend/src/editor/luxHighlight.ts
autonomous: true
requirements: [DSL-03]

must_haves:
  truths:
    - "Save button creates/updates preset via PUT /api/presets/<id> (not query param)"
    - "Share button copies URL-safe base64 link to clipboard"
    - "Share link decodes and loads effect into editor when opened via ?code= param"
    - "8 demo snippets appear in Идеи panel with Russian names and descriptions"
    - "for and frame keywords have help entries in Директивы section"
    - "for and frame keywords are syntax-highlighted in editor"
  artifacts:
    - path: "frontend/src/main.ts"
      provides: "Save route fix, share button handler, share link decode on load"
      contains: "api/presets/"
    - path: "frontend/src/ui/shellTemplate.ts"
      provides: "Share button markup in editor toolbar"
      contains: "share-button"
    - path: "frontend/src/editor/snippets.ts"
      provides: "8 new StarterSnippet entries"
      contains: "nyan-cat"
    - path: "frontend/src/editor/help.ts"
      provides: "for and frame help items"
      contains: "Цикл для рисования"
    - path: "frontend/src/editor/luxHighlight.ts"
      provides: "for and frame in LUX_KEYWORDS set"
      contains: '"for"'
  key_links:
    - from: "share button click → navigator.clipboard.writeText()"
      to: "browser clipboard"
      via: "URL-safe base64 encoding of editor content"
      pattern: "btoa"
    - from: "page load ?code= → atob() → setEditorValue()"
      to: "editor"
      via: "URL-safe base64 decode"
      pattern: "code="
    - from: "save button → fetch PUT /api/presets/<id>"
      to: "LampWebServer::handlePresetByPath()"
      via: "HTTP PUT with JSON body"
      pattern: "api/presets/"
</objective>

<execution_context>
@~/.copilot/get-shit-done/workflows/execute-plan.md
@~/.copilot/get-shit-done/templates/summary.md
</execution_context>

<context>
@.planning/phase-5-dsl/05-CONTEXT.md
@.planning/phase-5-dsl/05-RESEARCH.md
@.planning/phase-5-dsl/05-UI-SPEC.md
@frontend/src/main.ts
@frontend/src/ui/shellTemplate.ts
@frontend/src/editor/snippets.ts
@frontend/src/editor/help.ts
@frontend/src/editor/luxHighlight.ts

<interfaces>
From frontend/src/ui/shellTemplate.ts — editor toolbar pattern:
```html
<div class="editor-bar__actions">
  <button id="new-effect-button" type="button">Новый</button>
  <button id="validate-button" type="button">Проверить</button>
  <button id="run-button" type="button">Запустить</button>
  <button id="save-button" type="button">Сохранить</button>
</div>
```

From frontend/src/main.ts — save route (BROKEN):
```ts
const response = await fetch(`/api/presets?id=${encodeURIComponent(presetId)}`, { method: "PUT", ... });
```
Route MUST change to: `/api/presets/${encodeURIComponent(presetId)}` — path-based, not query param.

From frontend/src/main.ts — key functions:
- `getEditorValue(): string` — returns current editor content
- `setEditorValue(value: string)` — sets editor content
- `setText(id: string, text: string)` — sets element textContent
- `bindActionButtons()` — wires up button click handlers
- `savePreset()` — the function to fix (line ~747)

From frontend/src/editor/snippets.ts — snippet format:
```ts
export type StarterSnippet = { id: string; name: string; description: string; source: string; };
export const starterSnippets: StarterSnippet[] = [ ... 6 existing entries ... ];
```

From frontend/src/editor/help.ts — help format:
```ts
export type HelpItem = { term: string; description: string; };
export type HelpSection = { title: string; items: HelpItem[]; };
// "Директивы" section is the 3rd entry in editorHelpSections array
```

From frontend/src/editor/luxHighlight.ts — keyword set:
```ts
const LUX_KEYWORDS = new Set([
  "effect", "sprite", "text", "layer", "use", "color", "bitmap",
  "visible", "x", "y", "scale", "rotation", "blend",
]);
```
</interfaces>
</context>

<tasks>

<task type="auto">
  <name>Task 1: Fix save route + add share button handler</name>
  <files>frontend/src/main.ts, frontend/src/ui/shellTemplate.ts</files>
  <read_first>frontend/src/main.ts, frontend/src/ui/shellTemplate.ts, .planning/phase-5-dsl/05-UI-SPEC.md</read_first>
  <action>
    **Fix save route (per D-11):**
    In `savePreset()` function, change the fetch URL from query-param style to path-based:
    - BEFORE: `` `/api/presets?id=${encodeURIComponent(presetId)}` ``
    - AFTER: `` `/api/presets/${encodeURIComponent(presetId)}` ``
    - No other changes to `savePreset()` needed — method stays PUT, body stays same JSON, headers same
    - Route handler `LampWebServer::handlePresetByPath()` already reads the path segment and `server_.arg("plain")` for body

    **Add share button markup (per D-10, UI-SPEC §1):**
    In `shellTemplate.ts`, insert AFTER the save button:
    ```html
    <button id="share-button" type="button">Поделиться</button>
    ```
    New toolbar order: `Новый | Проверить | Запустить | Сохранить | Поделиться`
    No CSS changes — button inherits existing `.editor-bar__actions button` styles (pill, accent bg, hover lift).

    **Add share button handler in main.ts (per D-10, UI-SPEC §1):**
    In `bindActionButtons()`, add:
    ```ts
    const shareButton = document.getElementById("share-button") as HTMLButtonElement | null;
    shareButton?.addEventListener("click", () => { void handleShare(); });
    ```

    Add `handleShare()` function:
    ```ts
    async function handleShare(): Promise<void> {
      const source = getEditorValue().trim();
      if (!source) {
        setText("editor-status", "Нечего копировать: редактор пустой.");
        return;
      }
      try {
        // URL-safe base64: standard btoa, then replace +→-, /→_, strip =
        const base64 = btoa(unescape(encodeURIComponent(source)));
        const urlSafe = base64.replace(/\+/g, "-").replace(/\//g, "_").replace(/=+$/, "");
        const shareUrl = `${window.location.origin}${window.location.pathname}?code=${urlSafe}`;
        await navigator.clipboard.writeText(shareUrl);
        setText("editor-status", "Ссылка скопирована. Отправь её кому угодно.");
      } catch {
        setText("editor-status", "Не удалось скопировать ссылку.");
      }
    }
    ```

    **Add share link decode on page load (per D-10, UI-SPEC §2):**
    Add at the end of the initialization code (after `bindActionButtons()` call in the main init flow):
    ```ts
    // Decode share link if ?code= present
    const urlParams = new URLSearchParams(window.location.search);
    const codeParam = urlParams.get("code");
    if (codeParam) {
      try {
        // Reverse URL-safe base64
        let base64 = codeParam.replace(/-/g, "+").replace(/_/g, "/");
        while (base64.length % 4) base64 += "=";
        const source = decodeURIComponent(escape(atob(base64)));
        setEditorValue(source);
        setText("editor-status", "Эффект загружен по ссылке. Можно запустить или сохранить.");
      } catch {
        setText("editor-status", "Не удалось загрузить эффект по ссылке.");
      }
    }
    ```

    **Copywriting (all in Russian, per UI-SPEC):**
    - Share button label: "Поделиться"
    - Share success: "Ссылка скопирована. Отправь её кому угодно."
    - Share error: "Не удалось скопировать ссылку."
    - Decode success: "Эффект загружен по ссылке. Можно запустить или сохранить."
    - Decode error: "Не удалось загрузить эффект по ссылке."
  </action>
  <verify>
    <automated>grep -c "share-button" frontend/src/ui/shellTemplate.ts && grep -c "handleShare" frontend/src/main.ts && grep -c "api/presets/" frontend/src/main.ts | grep -v '?' && grep -c 'code=' frontend/src/main.ts</automated>
  </verify>
  <acceptance_criteria>
    - Save route uses `/api/presets/${encodeURIComponent(presetId)}` (no `?id=` query param)
    - Shell template has `<button id="share-button" type="button">Поделиться</button>` after save button
    - `handleShare()` function exists, encodes editor content as URL-safe base64, calls `navigator.clipboard.writeText()`
    - Share URL format: `{origin}{pathname}?code={url-safe-base64}`
    - On page load, `?code=` param decoded and loaded into editor via `setEditorValue()`
    - All status messages in Russian
    - `grep -c "share-button" frontend/src/ui/shellTemplate.ts` > 0
    - `grep -c "handleShare" frontend/src/main.ts` > 0
    - `grep -c 'code=' frontend/src/main.ts` > 0
    - `grep -c "api/presets/" frontend/src/main.ts` (without `?id=`) > 0
  </acceptance_criteria>
</task>

<task type="auto">
  <name>Task 2: Add 8 demo snippets + help entries + highlighting keywords</name>
  <files>frontend/src/editor/snippets.ts, frontend/src/editor/help.ts, frontend/src/editor/luxHighlight.ts</files>
  <read_first>frontend/src/editor/snippets.ts, frontend/src/editor/help.ts, frontend/src/editor/luxHighlight.ts, .planning/phase-5-dsl/05-UI-SPEC.md</read_first>
  <action>
    **Add 8 demo snippets (per D-06, D-07, D-09, UI-SPEC §4):**
    In `snippets.ts`, append 8 new `StarterSnippet` entries to the `starterSnippets` array after the existing "Часы" entry. Each entry follows the exact same format as existing ones.

    The `source` field should contain the full Lux DSL source matching the `.lux` files from Plan 05-03. Use the same source content so snippets and factory presets are consistent.

    Entries (in order):
    1. `id: "nyan-cat"`, `name: "Нян Кот"`, `description: "Котик с радужным шлейфом бежит по кругу."`
    2. `id: "mario"`, `name: "Марио"`, `description: "8-bit Марио шагает по окружности цилиндра."`
    3. `id: "plasma"`, `name: "Плазма"`, `description: "Абстрактные переливы — sin, cos и немного магии."`
    4. `id: "scrolling-text"`, `name: "Бегущая строка"`, `description: "Текст движется по окружности, как на стадионе."`
    5. `id: "snake"`, `name: "Змейка"`, `description: "Классическая змейка на цилиндре — игровая логика на DSL."`
    6. `id: "fire-particles"`, `name: "Огоньки"`, `description: "Частицы пламени поднимаются вверх."`
    7. `id: "starfield"`, `name: "Звёздное поле"`, `description: "Пролетающие звёзды — как в космосе."`
    8. `id: "dna"`, `name: "Спираль ДНК"`, `description: "Двойная спираль крутится на матрице."`

    **Add help entries (per UI-SPEC §5):**
    In `help.ts`, find the "Директивы" section (3rd entry in `editorHelpSections`). Append 2 new `HelpItem` entries to its `items` array:
    - `{ term: "for", description: "Цикл для рисования повторяющихся фигур. Всё внутри { } повторится несколько раз." }`
    - `{ term: "frame", description: "Выбор кадра анимации внутри sprite. Меняется по выражению: frame = (t * 4) % 3." }`

    **Add highlighting keywords (per UI-SPEC §6):**
    In `luxHighlight.ts`, add `"for"` and `"frame"` to the `LUX_KEYWORDS` Set. No other changes needed — these will be highlighted with existing `.tok-keyword` class.
  </action>
  <verify>
    <automated>cd /home/ros2/mylamp && grep -c '"nyan-cat"' frontend/src/editor/snippets.ts && grep -c '"mario"' frontend/src/editor/snippets.ts && grep -c '"for"' frontend/src/editor/help.ts && grep -c '"frame"' frontend/src/editor/help.ts && grep -c '"for"' frontend/src/editor/luxHighlight.ts && grep -c '"frame"' frontend/src/editor/luxHighlight.ts</automated>
  </verify>
  <acceptance_criteria>
    - `starterSnippets` array has 14 entries (6 existing + 8 new)
    - New entries use ids: nyan-cat, mario, plasma, scrolling-text, snake, fire-particles, starfield, dna
    - All snippet names and descriptions in Russian
    - Each snippet's `source` is valid Lux DSL matching the corresponding .lux file
    - "Директивы" help section has `for` and `frame` entries with Russian descriptions
    - `LUX_KEYWORDS` Set includes `"for"` and `"frame"`
    - `grep -c '"nyan-cat"' frontend/src/editor/snippets.ts` == 1
    - `grep -c '"for"' frontend/src/editor/luxHighlight.ts` == 1
    - `grep -c '"frame"' frontend/src/editor/luxHighlight.ts` == 1
    - `grep -c '"for"' frontend/src/editor/help.ts` == 1
    - `grep -c '"frame"' frontend/src/editor/help.ts` == 1
  </acceptance_criteria>
</task>

</tasks>

<threat_model>
## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| URL query param `?code=` → frontend decode | Untrusted user-provided base64; must not execute or render as HTML |
| Editor content → `btoa()` | User-authored DSL source; base64 is purely transport encoding, no security boundary |
| Clipboard API | Browser permission; graceful fallback on denial |

## STRIDE Threat Register

| Threat ID | Category | Component | Disposition | Mitigation Plan |
|-----------|----------|-----------|-------------|-----------------|
| T-5-09 | Spoofing | Share link decode (main.ts) | mitigate | Decoded content is set as plain text in editor via `setEditorValue()` — no HTML rendering, no eval. Invalid base64 caught by try/catch. Content still goes through `validateSource()` before execution on device. |
| T-5-10 | Information Disclosure | Share link URL | accept | Share link contains user-authored effect source in base64 — this is the intended feature. No secrets, credentials, or PII in effect source. |
| T-5-11 | Denial of Service | Clipboard API | accept | Browser handles clipboard permission; on denial, status message shown, no crash. |

</threat_model>

<verification>
- [ ] Save button: edit effect, click "Сохранить", verify preset appears in list after refresh
- [ ] Share button: click "Поделиться", paste URL in new tab, verify editor loads same effect
- [ ] Share link decode: open `http://device.local/?code=<valid-base64>`, verify editor populated
- [ ] Share link error: open `http://device.local/?code=!!!invalid!!!`, verify status shows error
- [ ] Snippets: click "Идеи", verify 14 entries visible (6 old + 8 new)
- [ ] Help: open help panel, verify `for` and `frame` in "Директивы" section
- [ ] Highlighting: type `for` and `frame` in editor, verify keyword color applied
</verification>

<success_criteria>
- Save route uses PUT /api/presets/<id> (path-based, not query param)
- Share button copies URL-safe base64 link to clipboard
- Share link decodes and loads into editor on page load
- 8 demo snippets visible in Идеи panel with Russian names
- for/frame keywords highlighted in editor
- for/frame help entries in Директивы section
- All frontend changes use existing CSS — no new styles needed
</success_criteria>

<output>
After completion, create `.planning/phases/05-dsl/05-04-SUMMARY.md`
</output>

---

## Cross-Plan Verification

### Requirement Coverage

| Requirement | 05-01 | 05-02 | 05-03 | 05-04 |
|-------------|:-----:|:-----:|:-----:|:-----:|
| DSL-01 (демо-эффекты с анимацией спрайтов) | — | ● | ● | — |
| DSL-02 (расширение языка Lux) | ● | ● | — | — |
| DSL-03 (демо-эффекты как пресеты в веб-интерфейсе) | — | — | ● | ● |

### Decision Traceability

| Decision | Plan(s) | How Implemented |
|----------|---------|-----------------|
| D-01 (named frames in sprite) | 05-01, 05-02 | Token.h + Ast.h: SpriteFrameDeclaration; Lexer: multi-frame sprite body; Compiler: compileSpriteFrames() |
| D-02 (frame selection via expression) | 05-01, 05-02 | Ast.h: LayerDeclaration.frameExpression; Compiler: compile frame expr; Executor: evaluate + select frame |
| D-03 (for loops with compile-time unrolling) | 05-01, 05-02 | Ast.h: ForLoopStatement; Parser: parseForLoop(); Compiler: unrollForLoops() with string substitution |
| D-04 (backward compatibility) | 05-01, 05-02 | Lexer: old bitmap path preserved; Compiler/Executor: frames.empty() → use sprite.pixels |
| D-05 (layer.use stays sprite name) | 05-01 | Parser: frame is separate property, not part of use; use still matches kIdentifier |
| D-06 (required demos: 5 effects) | 05-03 | 5 .lux files: nyan-cat, mario, plasma, scrolling-text, snake |
| D-07 (additional 2-3 effects) | 05-03 | 3 .lux files: fire-particles, starfield, dna |
| D-08 (all demos in Lux DSL) | 05-03 | All 8 effects are .lux files, not C++ IEffect |
| D-09 (demos as snippets + presets) | 05-03, 05-04 | snippets.ts: 8 entries; main.cpp: seedFactoryPresets() |
| D-10 (share button base64 link) | 05-04 | main.ts: handleShare() + decode on load; shellTemplate.ts: share button markup |
| D-11 (fix preset save route) | 05-04 | main.ts savePreset(): PUT /api/presets/<id> (path-based) |

### Source Audit

All 4 source artifact types covered:

**GOAL (ROADMAP):** "Добавлены крутые демо-эффекты (спрайтовая анимация), при необходимости расширен Lux DSL."
→ Covered by Plans 05-01 (DSL extension), 05-02 (execution), 05-03 (demos)

**REQ (REQUIREMENTS.md):** DSL-01, DSL-02, DSL-03 — all three covered as shown in table above.

**RESEARCH (05-RESEARCH.md):** All feature recommendations covered: compile-time unrolling (05-02), URL-safe base64 (05-04), path-based PUT (05-04), MAX_UNROLLED_LAYERS=64 (05-02), factory preset seeding on empty /presets/ (05-03), backward-compatible sprite extension (05-01, 05-02).

**CONTEXT (05-CONTEXT.md):** All 11 locked decisions (D-01–D-11) traced to specific plan tasks. No deferred ideas implemented. Agent discretion areas resolved: comparison operators `<`, `<=`, `>`, `>=` all supported; frame ordering index-based with user names; URL-safe base64 (RFC 4648 §5); 3 additional effects: Fire, Starfield, DNA.

---

## Global Verification

- [ ] `platformio test -e native -v` — full test suite passes (no regressions)
- [ ] Flash dev build to device, verify:
  - [ ] All 8 factory presets appear in web UI preset list on first boot
  - [ ] Each demo effect runs and renders correctly on cylinder
  - [ ] Nyan Cat shows walking animation with rainbow trail
  - [ ] Mario walks around the cylinder
  - [ ] Save button creates persistent preset
  - [ ] Share button copies valid URL
  - [ ] Share link decodes and loads effect in editor
  - [ ] for/frame keywords highlighted in editor
  - [ ] Help panel shows for/frame entries
</success_criteria>

<output>
After all plans complete, create `.planning/phases/05-dsl/05-SUMMARY.md`
</output>
