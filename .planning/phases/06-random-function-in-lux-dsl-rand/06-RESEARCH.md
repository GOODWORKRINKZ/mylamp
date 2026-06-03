# Phase 6: Выразительность Lux DSL — Research

**Researched:** 2026-06-03
**Status:** Complete

## Research Questions

### RQ1: Как добавить random/randf в ExpressionCompiler/Executor?

**Текущая архитектура:**
- `ExpressionOp` — enum в `CompiledProgram.h` (25 значений)
- `buildFunction()` в `Compiler.cpp:218` — цепочка `if/else if`, обрабатывает `temp`, `humidity`, `sin`, `cos`, `abs`, `min`, `max`, `clamp`, `mix`, `smoothstep`
- `evaluateNode()` в `Executor.cpp:34` — switch по `ExpressionOp`, рекурсивный обход дерева
- `ExpressionCompiler` — рекурсивный спуск: `parseExpression → parseTerm → parseFactor → parseUnary → parseIdentifier`

**Решение для random/randf:**
1. `ExpressionOp`: +`kRandom`, +`kRandf` (перед `kLoopIndex`)
2. `buildFunction()`: проверить `name == "random"` / `name == "randf"`, проверить `arguments.size() == 1U`, `children[0] = arguments[0]`
3. `evaluateNode()`: 
   - `kRandom`: `(esp_random() / 4294967295.0f) * evaluateNode(nodes, children[0], ...)`
   - `kRandf`: ленивое кэширование через `float* frameRandCache`. `isnan(cache[index])` → вычислить и сохранить; иначе вернуть кэш
4. `evaluateNode()` — сигнатура меняется: добавить `float* frameRandCache = nullptr`
5. `Executor::render()` — создать `std::vector<float> frameRandCache(nodes.size(), NAN)` и передать в `evaluateNode()`
6. `ExecutionContext` — не требует изменений (randf кэш per-frame, не per-pixel)

**Риски:** `esp_random()` возвращает `uint32_t`. Деление на `UINT32_MAX` даёт `[0, 1)`. Умножение на `max` даёт `[0, max)`. Защита от `max <= 0` — возвращать `0.0f`.

---

### RQ2: Как добавить операторы сравнения и if() в ExpressionCompiler?

**Текущая грамматика выражений:**
```
expression → term (('+' | '-') term)*
term       → factor (('*' | '/' | '%') factor)*
factor     → ('-')? unary
unary      → number | identifier | '(' expression ')' | function_call
```

**Нужна расширенная грамматика:**
```
expression → and_expr
and_expr   → comparison (('&&') comparison)*
comparison → term (('>' | '<' | '>=' | '<=' | '==' | '!=') term)*
term       → factor (('+' | '-') factor)*
factor     → unary (('*' | '/' | '%') unary)*
unary      → ('-' | '!')? primary
primary    → number | identifier | '(' expression ')' | function_call
```

**Реализация:**
1. `ExpressionOp`: +`kGt`, `kLt`, `kGte`, `kLte`, `kEq`, `kNeq`, `kAnd`, `kNot`, `kIf`
2. Новый метод `parseAnd()` — обрабатывает `&&`
3. Новый метод `parseComparison()` — обрабатывает `> < >= <= == !=`
4. `parseExpression()` теперь вызывает `parseAnd()` вместо `parseTerm()`
5. `parseUnary()` — добавить обработку `!`
6. `kNot` — унарный: `children[0]` = операнд, возвращает `1.0f - evaluateNode(...)`
7. `kAnd` — бинарный: `children[0]`, `children[1]`. В evaluate: `a != 0 && b != 0 ? 1.0f : 0.0f`
8. `buildFunction()`: +`if` — проверка `arguments.size() == 3U`, `children[0..2]`

**Приоритеты (от низшего к высшему):**
1. `&&` (низший)
2. `> < >= <= == !=`
3. `+ -`
4. `* / %`
5. `- !` (унарные)

**Риски:** `==` и `!=` с float — эпсилон-сравнение? Решение: точное сравнение (как в GLSL). `a == b` → `fabs(a - b) < 0.00001f ? 1.0f : 0.0f`.

---

### RQ3: Как реализовать compute-блоки с let/while?

**Ключевой вопрос:** compute-блоки — это новый вид AST-узлов, не expressions. Как их скомпилировать в `CompiledProgram`?

**Дизайн:**
```
compute iter {
    let cx = nx * 3 - 2
    let zx = 0
    while (zx * zx < 4 && i < 64) {
        zx = zx * zx + cx
        i = i + 1
    }
    i
}
```

**AST-расширения (`Ast.h`):**
```cpp
enum class StatementKind { kLet, kAssign, kWhile, kExpr };

struct Statement {
    StatementKind kind;
    std::string varName;          // для Let/Assign
    std::string expression;       // строковое выражение (компилируется позже)
    std::string condition;        // для While
    std::vector<Statement> body;  // для While
};

struct ComputeBlock {
    std::string name;
    std::vector<Statement> body;
};
// Добавить в Program: std::vector<ComputeBlock> computes;
```

**Лексер (`Lexer.cpp`):**
- Новые ключевые слова: `compute`, `let`, `while`
- Новые токены: `kKeywordCompute`, `kKeywordLet`, `kKeywordWhile`
- `compute name {` — парсится как block-header (аналогично `sprite`, `layer`)
- Внутри `compute`-блока: `let x = expr`, `x = expr`, `while (cond) { ... }`
- `{` / `}` — используются для тела while

**Парсер (`Parser.cpp`):**
- `parseCompute()` — новый метод, вызывается из `parseProgram()` при `kKeywordCompute`
- `parseStatement()` — разбирает `let`, присваивание, `while`, выражение
- `parseWhile()` — разбирает условие и тело

**Компилятор (`Compiler.cpp`):**
Compute-блок компилируется в плоский массив `ExpressionNode` с новым `ExpressionOp::kComputeResult`:
1. Для каждой `let`-переменной — создаётся узел `kStore` с индексом в "регистровом файле"
2. `while` компилируется в узел `kWhile` с `children[0]` = условие, `children[1]` = тело
3. Тело while — последовательность узлов: `kStore` для присваиваний
4. Результат блока — последнее выражение, сохраняется в `kComputeResult`

**Альтернатива (проще):** Compute-блок выполняется интерпретатором на этапе `evaluateNode()`:
- Переменные хранятся в `std::unordered_map<std::string, float>` — per-pixel стек
- `let x = expr` → вычислить expr, сохранить в map
- `x = expr` → вычислить expr, обновить map
- `while (cond) { body }` → while-петля: eval cond, если != 0 → выполнить body, повторить
- Результат: eval последнего выражения

**Решение:** интерпретируемый подход проще и не требует сложной компиляции. Минус — медленнее, но для 512 пикселей × 64 итераций = 32K операций — терпимо на ESP32-C3.

**Execution flow:**
1. `Executor::render()` — для каждого пикселя:
   a. Создать `std::unordered_map<std::string, float> computeVars`
   b. Для каждого compute-блока: выполнить его тело, сохранить результат в `computeVars[blockName]`
   c. При вычислении layer-выражений: `computeVars` доступен через `EvaluationContext` или отдельный параметр
2. `evaluateNode()` — для `kComputeRef` вернуть значение из `computeVars`

**Риски:**
- Глубина рекурсии while: ограничена `kMaxExpressionDepth` (100 по умолчанию). 64 итерации Мандельброта — ок.
- Переменные перекрывают встроенные (`t`, `x`, `nx`)? Решение: compute-переменные в отдельном неймспейсе. При конфликте — ошибка компиляции.
- `while` не должен быть бесконечным: лимит итераций = `kMaxExpressionDepth`.

---

### RQ4: Как интегрировать compute-переменные в layer-выражения?

**Проблема:** `layer` использует `ExpressionCompiler` который знает про `t`, `x`, `nx` и т.д., но не про compute-переменные.

**Решение:** compute-переменные добавляются в `parseIdentifier()` как ещё один источник:
```cpp
if (name == "t") { node.op = kTime; }
else if (name == "x") { node.op = kCoordX; }
// ...
else if (computeVars.count(name)) { node.op = kComputeRef; node.constant = computeIndex; }
else { error("Неизвестный идентификатор"); }
```

Но `ExpressionCompiler` компилирует ДО выполнения — он не знает имён compute-переменных на этапе компиляции. Нужно:
1. Собрать имена всех compute-переменных ДО компиляции layer-выражений
2. Передать их в `ExpressionCompiler` как допустимые идентификаторы
3. `ExpressionCompiler` создаёт `kComputeRef` узел с индексом переменной

**Implementation:**
- `CompiledProgram` получает `std::vector<std::string> computeNames` — имена compute-блоков
- `ExpressionCompiler` получает `const std::vector<std::string>& computeNames`
- В `parseIdentifier()`: если имя есть в `computeNames` → `kComputeRef` с индексом
- `evaluateNode()` для `kComputeRef`: вернуть значение из `computeResults[child_index]`

---

### RQ5: Какие изменения нужны в лексере/парсере?

**Token.h:**
```cpp
kKeywordCompute,  // compute
kKeywordLet,      // let
kKeywordWhile,    // while
```

**Lexer.cpp — новые правила (добавить в tokenize() ДО propertyRules):**
```
if (startsWith(trimmed, "compute ")) → parseBlockHeader → kKeywordCompute + kIdentifier + kLeftBrace
if (startsWith(trimmed, "let "))     → kKeywordLet + kIdentifier + kEquals + kExpression
if (startsWith(trimmed, "while ("))  → kKeywordWhile + parse condition + kLeftBrace
```

**Parser.cpp — parseProgram():**
После обработки `kKeywordFor`, добавить:
```cpp
if (state.current().type == TokenType::kKeywordCompute) {
    state.match(TokenType::kKeywordCompute);
    parseCompute(state, program, diagnostics);
    continue;
}
```

**Parser.cpp — parseCompute():**
- Имя блока: `expect(kIdentifier)`
- `expect(kLeftBrace)`
- Цикл: пока не `kRightBrace` — вызывать `parseStatement()`
- Сохранить в `program.computes`

---

## Validation Architecture

### Критические проверки (must-pass):
1. `test_dsl_random`: random() в [0, max), randf() одинаков для всех пикселей в кадре
2. `test_dsl_conditional`: if(), сравнения, &&, ! — корректные значения
3. `test_dsl_compute`: let/while/assign — корректный Mandelbrot-результат
4. Обратная совместимость: все существующие тесты проходят

### Nyquist dimensions:
- D1 (Unit): все новые ExpressionOp покрыты тестами
- D2 (Integration): compute → layer pipeline
- D3 (Edge): max ≤ 0, деление на 0, переполнение while
- D8 (Validation): Mandelbrot на реальном устройстве

---

## Files to Modify (сводка)

| File | Changes |
|------|---------|
| `include/live/runtime/CompiledProgram.h` | +11 ExpressionOp, +ComputeBlock в CompiledProgram |
| `include/live/dsl/Token.h` | +kKeywordCompute, +kKeywordLet, +kKeywordWhile |
| `include/live/dsl/Ast.h` | +ComputeBlock, +Statement, +StatementKind |
| `include/live/runtime/Executor.h` | evaluateNode с frameRandCache, computeResults |
| `src/live/runtime/Compiler.cpp` | ExpressionCompiler: parseAnd, parseComparison, !, computeNames; buildFunction: random, randf, if |
| `src/live/runtime/Executor.cpp` | evaluateNode: все новые ExpressionOp, while-петля |
| `src/live/dsl/Lexer.cpp` | tokenize: compute, let, while правила |
| `src/live/dsl/Parser.cpp` | parseCompute, parseStatement, parseWhile |
| `frontend/src/editor/luxHighlight.ts` | +randf, compute, let, while в LUX_KEYWORDS/FUNCTIONS |
| `docs/DSL.md` | Новые секции: random, randf, if, сравнения, compute |
| `test/test_dsl_random/` | Новый тестовый таргет |
| `test/test_dsl_conditional/` | Новый тестовый таргет |
| `test/test_dsl_compute/` | Новый тестовый таргет |
| `platformio.ini` | +3 test environments |
