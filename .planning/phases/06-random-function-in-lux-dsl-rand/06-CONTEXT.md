# Phase 6: Выразительность Lux DSL — Context

**Gathered:** 2026-06-03
**Status:** Ready for planning

<domain>
## Phase Boundary

Расширить Lux DSL для поддержки генеративных эффектов и фракталов. Три группы фич:

1. **Random:** `random(max)` per-pixel + `randf(max)` per-frame (Strudel-паттерн)
2. **Условия:** `if(cond, a, b)`, операторы `> < >= <= == !=`, логические `&&` `!`
3. **Императив:** `compute name { let... while... }` — per-pixel вычислительные блоки

Целевой use-case: множество Мандельброта на чистом Lux DSL.

</domain>

<decisions>
## Implementation Decisions

---

### Часть 1: Random — `random(max)` и `randf(max)`

| D-# | Решение |
|-----|---------|
| D-01 | `random(max)` — per-pixel, `esp_random()` при каждом вызове `evaluateNode()`. `[0, max)`. |
| D-02 | `randf(max)` — per-frame (f=frame). Один вызов `esp_random()` на кадр, кэшируется. |
| D-03 | Сигнатура: один аргумент, как `sin(value)`. |
| D-04 | Дизайн-референс: Strudel `choose`/`chooseCycles`. |
| D-05 | Каждый `randf()`-вызов — независимый per-frame слот (как uniform-ы). |
| D-06 | Кэш: `float* frameRandCache` размера `nodes.size()`, NaN-инициализация. `isnan(cache[i])` → вычислить и сохранить. |
| D-07 | `ExpressionOp`: `kRandom`, `kRandf`. |

---

### Часть 2: Условный оператор и сравнения

| D-# | Решение |
|-----|---------|
| D-08 | `if(cond, a, b)` — функция с 3 аргументами. `cond != 0.0` → `a`, иначе `b`. |
| D-09 | Операторы: `>`, `<`, `>=`, `<=`, `==`, `!=`. Возвращают `0.0` / `1.0` (GLSL-стиль). |
| D-10 | Логические: `&&` (AND), `!` (NOT). `||` — потом (можно через `a + b - a*b`). |
| D-11 | Приоритет: сравнения < арифметика. `a + b > c * d` = `(a+b) > (c*d)`. |
| D-12 | `ExpressionOp`: `kGt`, `kLt`, `kGte`, `kLte`, `kEq`, `kNeq`, `kAnd`, `kNot`, `kIf`. |
| D-13 | `ExpressionCompiler`: новый уровень `parseComparison()` + `parseAnd()`. Грамматика: `and → comparison (&& comparison)*`, `comparison → term (('>'|'<'|'>='|'<='|'=='|'!=') term)*` |

---

### Часть 3: Императивный compute-блок

#### Синтаксис
```txt
compute iter {
    let cx = nx * 3 - 2
    let cy = ny * 2 - 1
    let zx = 0
    let zy = 0
    let i = 0
    while (i < 64 && zx*zx + zy*zy <= 4) {
        let xt = zx*zx - zy*zy + cx
        zy = 2*zx*zy + cy
        zx = xt
        i = i + 1
    }
    i       // ← результат блока
}

layer fractal {
    use dot
    color rgb(iter * 4, iter * 2, iter * 4)
}
```

| D-# | Решение |
|-----|---------|
| D-14 | `compute name { ... }` — именованный per-pixel блок. Имя = переменная в layer-ах. |
| D-15 | `let x = expr` — объявление изменяемой per-pixel переменной. |
| D-16 | `x = expr` — переприсваивание существующей переменной. |
| D-17 | `while (cond) { ... }` — цикл с условием. Допустим только в `compute`. |
| D-18 | Результат блока — последнее выражение (без `;`). Тип: float. |
| D-19 | Переменные `compute`-блока видны во всех `layer`-ах этого эффекта. |
| D-20 | Несколько `compute`-блоков: каждый — отдельная переменная. |

#### AST и компиляция
| D-# | Решение |
|-----|---------|
| D-21 | Новый AST-узел: `ComputeBlock { name, body: vector<Statement> }` |
| D-22 | `Statement` = `Let { name, expr }` | `Assign { name, expr }` | `While { cond, body }` | `Expr { expr }` |
| D-23 | `compute`-блок компилируется в набор `ExpressionNode`. Итоговое значение — `ExpressionOp::kComputeRef` с индексом. |
| D-24 | `while` компилируется в цикл внутри `evaluateNode()`: рекурсивный вызов тела пока `cond != 0`. |
| D-25 | Глубина рекурсии `while`: ограничена `kMaxExpressionDepth` (уже есть). Для Мандельброта: 64 итерации × ~10 узлов = 640, в пределах лимита. |

#### Парсинг
| D-# | Решение |
|-----|---------|
| D-26 | `compute` — новое ключевое слово верхнего уровня (как `sprite`, `layer`). |
| D-27 | `let`, `while` — ключевые слова внутри `compute`-блока. |
| D-28 | `{` `}` — блоки. Внутри `compute`: последовательность statement-ов. |

---

### Обратная совместимость
| D-# | Решение |
|-----|---------|
| D-29 | Все существующие `.lux`-эффекты работают без изменений. |
| D-30 | Новые ключевые слова (`compute`, `let`, `while`) не конфликтуют с именами sprite/layer. |

### Scope (что в Phase 6)
- ✅ `random(max)`, `randf(max)` — random-функции
- ✅ `if(cond, a, b)` — условное выражение
- ✅ `> < >= <= == !=` — сравнения
- ✅ `&&`, `!` — логические
- ✅ `compute name { let... while... }` — императивный блок
- ❌ `||` — later (выразимо через арифметику)
- ❌ `break` — later
- ❌ `for` для вычислений — later (while + let достаточно)

### the agent's Discretion
- ExpressionOp: +11 значений (kRandom, kRandf, kGt, kLt, kGte, kLte, kEq, kNeq, kAnd, kNot, kIf)
- AST: новый `ComputeBlock`, `Statement` (Let/Assign/While/Expr)
- Lexer: +`compute`, `let`, `while` ключевые слова
- Parser: +`parseCompute()`, +`parseStatement()`, +`parseWhile()`
- ExpressionCompiler: +`parseComparison()`, +`parseAnd()`
- Executor: `evaluateNode()` с `frameRandCache`, `while`-петля с учётом `kMaxExpressionDepth`
- Фронтенд: `luxHighlight.ts` — добавить `randf`, `compute`, `let`, `while`
- Документация: `docs/DSL.md` — все новые конструкции
- Тесты: `test_dsl_random`, `test_dsl_conditional`, `test_dsl_compute`
