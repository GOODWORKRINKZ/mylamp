# Phase 6: Выразительность Lux DSL — Plan

**Created:** 2026-06-03
**Phase:** 06-random-function-in-lux-dsl-rand
**Waves:** 4

---

## Plan 06-01: ExpressionOps — random, сравнения, if, &&, !

**Purpose:** Добавить 11 новых ExpressionOp, расширить ExpressionCompiler, buildFunction, evaluateNode.
**Depends on:** Phase 5 (DSL base)

### Tasks

| # | Task | Files | Type |
|---|------|-------|------|
| 1.1 | Добавить ExpressionOp: kRandom, kRandf, kGt, kLt, kGte, kLte, kEq, kNeq, kAnd, kNot, kIf | `CompiledProgram.h` | enum |
| 1.2 | ExpressionCompiler: parseComparison() — парсинг `> < >= <= == !=` | `Compiler.cpp` | new method |
| 1.3 | ExpressionCompiler: parseAnd() — парсинг `&&` | `Compiler.cpp` | new method |
| 1.4 | ExpressionCompiler: parseUnary() — поддержка `!` | `Compiler.cpp` | modify |
| 1.5 | ExpressionCompiler: parseExpression() → вызывает parseAnd() вместо parseTerm() | `Compiler.cpp` | modify |
| 1.6 | buildFunction(): ветки random, randf, if | `Compiler.cpp` | modify |
| 1.7 | evaluateNode(): kRandom, kRandf (с frameRandCache), kGt-kNeq, kAnd, kNot, kIf | `Executor.cpp` | modify |
| 1.8 | evaluateNode(): сигнатура + float* frameRandCache | `Executor.h`, `Executor.cpp` | modify |
| 1.9 | Executor::render(): создать frameRandCache, передать в evaluateNode | `Executor.cpp` | modify |
| 1.10 | Написать тесты: test_dsl_random | `test/test_dsl_random/` | new |
| 1.11 | Написать тесты: test_dsl_conditional | `test/test_dsl_conditional/` | new |
| 1.12 | platformio.ini: добавить test environments | `platformio.ini` | modify |

### Verification
- `pio test -e native-test --filter test_dsl_random` — все тесты проходят
- `pio test -e native-test --filter test_dsl_conditional` — все тесты проходят
- Существующие тесты (`test_dsl_*`, `test_effects`, `test_live_api`) не сломаны

---

## Plan 06-02: AST + Lexer/Parser — compute, let, while

**Purpose:** Новые AST-узлы, ключевые слова, парсинг compute-блоков.
**Depends on:** Plan 06-01 (ExpressionOps)

### Tasks

| # | Task | Files | Type |
|---|------|-------|------|
| 2.1 | Token.h: +kKeywordCompute, +kKeywordLet, +kKeywordWhile | `Token.h` | enum |
| 2.2 | Ast.h: StatementKind, Statement, ComputeBlock | `Ast.h` | struct |
| 2.3 | Program: +computes vector | `Ast.h` | modify |
| 2.4 | Lexer: tokenize compute-блоков (`compute name {`) | `Lexer.cpp` | new rule |
| 2.5 | Lexer: tokenize let-операторов (`let x = expr`) | `Lexer.cpp` | new rule |
| 2.6 | Lexer: tokenize while-циклов (`while (cond) {`) | `Lexer.cpp` | new rule |
| 2.7 | Lexer: tokenize переприсваивания (`x = expr`) внутри compute | `Lexer.cpp` | new rule |
| 2.8 | Parser: parseCompute() — разбор compute-блока | `Parser.cpp` | new method |
| 2.9 | Parser: parseStatement() — let/assign/while/expr | `Parser.cpp` | new method |
| 2.10 | Parser: parseWhile() — условие + тело | `Parser.cpp` | new method |
| 2.11 | Parser: parseProgram() — вызов parseCompute при kKeywordCompute | `Parser.cpp` | modify |
| 2.12 | Написать тесты: test_dsl_compute (парсинг) | `test/test_dsl_compute/` | new |
| 2.13 | platformio.ini: +test_dsl_compute | `platformio.ini` | modify |

### Verification
- `pio test -e native-test --filter test_dsl_compute` — парсинг compute/let/while корректен
- `pio test -e native-test --filter test_dsl_parser` — существующие тесты проходят

---

## Plan 06-03: Executor — compute-выполнение и интеграция

**Purpose:** Выполнение compute-блоков (let/while/assign) + compute-переменные в layer-выражениях.
**Depends on:** Plan 06-02 (AST + Parser)

### Tasks

| # | Task | Files | Type |
|---|------|-------|------|
| 3.1 | CompiledProgram: +computeNames, +computeNodeIndices | `CompiledProgram.h` | modify |
| 3.2 | Compiler: compileCompute() — компиляция ComputeBlock → ExpressionNodes | `Compiler.cpp` | new method |
| 3.3 | Compiler: compileProgram() — обработка computes | `Compiler.cpp` | modify |
| 3.4 | ExpressionCompiler: поддержка computeNames в parseIdentifier() | `Compiler.cpp` | modify |
| 3.5 | ExpressionOp: +kComputeRef (ссылка на compute-переменную) | `CompiledProgram.h` | enum |
| 3.6 | evaluateNode(): kComputeRef — возврат из computeResults | `Executor.cpp` | modify |
| 3.7 | Executor::render(): выполнение compute-блоков перед рендерингом слоёв | `Executor.cpp` | modify |
| 3.8 | Executor: evaluateComputeBlock() — интерпретация let/assign/while | `Executor.cpp` | new method |
| 3.9 | Executor: лимит итераций while = kMaxExpressionDepth | `Executor.cpp` | guard |
| 3.10 | Executor: проверка конфликта имён (compute vs builtin: t, x, nx...) | `Executor.cpp` | validation |
| 3.11 | Расширить тесты test_dsl_compute: выполнение Mandelbrot-выражения | `test/test_dsl_compute/` | modify |
| 3.12 | Интеграционный тест: полный Mandelbrot (compute → layer → render) | `test/test_dsl_compute/` | new |

### Verification
- `pio test -e native-test --filter test_dsl_compute` — Mandelbrot-вычисление корректно
- `pio test -e native-test --filter test_dsl_executor` — обратная совместимость
- `pio test -e native-test --filter test_live_api` — не сломано

---

## Plan 06-04: Frontend + Docs + UAT

**Purpose:** Обновить подсветку, документацию, сниппеты. Визуальное UAT на устройстве.
**Depends on:** Plan 06-03 (Executor)

### Tasks

| # | Task | Files | Type |
|---|------|-------|------|
| 4.1 | luxHighlight.ts: +randf, +if в LUX_FUNCTIONS; +compute, +let, +while в LUX_KEYWORDS | `luxHighlight.ts` | modify |
| 4.2 | luxHighlight.ts: +`> < >= <= == != && !` — подсветка операторов | `luxHighlight.ts` | modify |
| 4.3 | docs/DSL.md: секции random/randf, if/сравнения, compute/let/while | `docs/DSL.md` | modify |
| 4.4 | docs/DSL.md: пример Мандельброта | `docs/DSL.md` | modify |
| 4.5 | Сниппет: Mandelbrot в starterSnippets | `snippets.ts` | modify |
| 4.6 | Прошивка dev-сборки, визуальное UAT | device | manual |
| 4.7 | UAT: Mandelbrot рендерится на матрице 32×16 | device | verify |
| 4.8 | UAT: random-эффект (звёздное небо) работает | device | verify |

### Verification
- `pio run -e esp32-c3-supermini-dev --target upload` — прошивка успешна
- Мандельброт визуально узнаваем на цилиндре
- Звёздное небо (random per-pixel) без артефактов
- Подсветка синтаксиса в редакторе корректна для новых ключевых слов

---

## Wave Dependencies

```
Wave 0: Plan 06-01 (ExpressionOps)
  ↓
Wave 1: Plan 06-02 (AST + Parser)
  ↓
Wave 2: Plan 06-03 (Executor + Integration)
  ↓
Wave 3: Plan 06-04 (Frontend + Docs + UAT)
```

---

## Threat Model (STRIDE)

| # | Threat | Severity | Mitigation |
|---|--------|----------|------------|
| T1 | Бесконечный while-loop → зависание устройства | HIGH | Лимит итераций = kMaxExpressionDepth (100). При превышении — возврат 0.0f + diagnostic. |
| T2 | Переполнение стека при глубокой рекурсии evaluateNode | MEDIUM | Существующий depth guard. while-тело увеличивает depth на 1 за итерацию. |
| T3 | Деление на ноль в random(max)/randf(max) при max=0 | LOW | Защита: если max ≤ 0.0f → return 0.0f |
| T4 | Конфликт имён compute-переменных со встроенными (t, x, nx...) | MEDIUM | Валидация при компиляции: если compute-переменная ≡ builtin → diagnostic error |
| T5 | esp_random() может вернуть одинаковые значения для всех пикселей | LOW | Маловероятно (HW RNG). Не блокирующая проблема — визуально заметно, но не фатально. |

---

## Verification Items

- [ ] V-01: `random(max)` возвращает значения в [0, max), разные для каждого вызова
- [ ] V-02: `randf(max)` возвращает одно значение для всех пикселей в кадре
- [ ] V-03: Несколько randf()-вызовов дают независимые per-frame значения
- [ ] V-04: `if(cond, a, b)` возвращает a при cond ≠ 0, иначе b
- [ ] V-05: `> < >= <= == !=` возвращают 0.0/1.0 корректно
- [ ] V-06: `&&` и `!` работают как логические операторы
- [ ] V-07: Приоритеты: арифметика > сравнения > && (a + b > c && d → ((a+b) > c) && d)
- [ ] V-08: compute-блок: let/assign/while выполняются корректно
- [ ] V-09: compute-переменные доступны в layer-выражениях
- [ ] V-10: Мандельброт вычисляется корректно (проверка известных точек)
- [ ] V-11: while не зависает (лимит итераций)
- [ ] V-12: Все существующие тесты проходят (обратная совместимость)
- [ ] V-13: Подсветка синтаксиса в редакторе для новых ключевых слов
- [ ] V-14: Мандельброт визуально узнаваем на LED-матрице

---

*Plan: 06-Plan*
*Created: 2026-06-03*
