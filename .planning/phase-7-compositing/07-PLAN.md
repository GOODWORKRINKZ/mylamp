# Phase 7: Layer Compositing & Blend — Plan

**Created:** 2026-06-03
**Status:** Ready for execution
**Based on:** 07-CONTEXT.md

---

## Task Dependency Graph

```
T1 (screen blend) ─────────────────────────────────────────┐
T2 (ClockConfig structs) ──┐                                │
T3 (clock block parsing) ──┤                                │
T4 (clock block compiling) ┘                                │
     ↓                                                       │
T5 (Z-sorting in Executor) ← depends on T1, T2              │
     ↓                                                       │
T6 (ClockOverlay → blend + alpha) ← depends on T1, T2      │
     ↓                                                       │
T7 (main.cpp: remove isEffectOnTop) ← depends on T5        │
     ↓                                                       │
T8 (frontend: help + highlight fix) ← depends on T1 ───────┘
     ↓
T9 (update tests)
     ↓
T10 (update docs DSL.md + LLM_EFFECT_PROMPT.md)
```

**T1, T2, T3, T4 можно делать параллельно.** T5 требует T1 и T2. T6 требует T1 и T2. T8 зависит только от T1.

---

## T1: Добавить `screen` blend mode

### T1.1 — Enum
**Файл:** `include/live/runtime/CompiledProgram.h`
- Добавить `kScreen` в `enum class BlendMode` после `kMultiply`

### T1.2 — Компиляция
**Файл:** `src/live/runtime/Compiler.cpp`
- В `compileBlendMode()`: добавить `if (trimmed == "screen") { blendMode = BlendMode::kScreen; return true; }`
- Обновить diagnostic-сообщение: `"Поддерживаются blend режимы: normal, add, multiply, screen"`

### T1.3 — Выполнение
**Файл:** `src/live/runtime/Executor.cpp`
- В `blendColors()`: добавить case `BlendMode::kScreen`:
  ```cpp
  case BlendMode::kScreen:
    return lamp::Rgb{
        static_cast<uint8_t>(255U - ((255U - destination.r) * (255U - source.r)) / 255U),
        static_cast<uint8_t>(255U - ((255U - destination.g) * (255U - source.g)) / 255U),
        static_cast<uint8_t>(255U - ((255U - destination.b) * (255U - source.b)) / 255U),
    };
  ```

### T1.4 — Подсветка синтаксиса
**Файл:** `frontend/src/editor/luxHighlight.ts`
- Добавить `"screen"` в массив ключевых слов (строка 19, где уже `"normal", "add", "multiply", "screen"` — screen уже там! Проверить, если уже есть — ок, если нет — добавить)

---

## T2: Структуры ClockConfig

### T2.1 — AST: ClockBlock
**Файл:** `include/live/dsl/Ast.h`
- Добавить в namespace `lamp::live::dsl`:
  ```cpp
  struct ClockBlock {
    bool hasBlock = false;
    std::string enabledExpression;   // "0" or "1"
    std::string zExpression;         // "1" etc
    std::string blendMode;           // "normal", "add", ...
    std::string alphaExpression;     // "1.0" etc
    uint32_t enabledLine = 0;
    uint32_t zLine = 0;
    uint32_t blendLine = 0;
    uint32_t alphaLine = 0;
  };
  ```
- Добавить поле `ClockBlock clockBlock;` в `struct Program`

### T2.2 — Compiled: ClockConfig
**Файл:** `include/live/runtime/CompiledProgram.h`
- Добавить:
  ```cpp
  struct CompiledClockConfig {
    bool enabled = true;
    int16_t zExpression = -1;        // -1 = use default z=1
    BlendMode blendMode = BlendMode::kNormal;
    int16_t alphaExpression = -1;    // -1 = use default alpha=1.0
  };
  ```
- Добавить поле `CompiledClockConfig clockConfig;` в `struct CompiledProgram`

### T2.3 — ExecutionContext
**Файл:** `include/live/runtime/Executor.h`
- Добавить `CompiledClockConfig clockConfig;` в `struct ExecutionContext`

---

## T3: Парсинг `clock { ... }` блока

### T3.1 — Новый токен
**Файл:** `include/live/dsl/Token.h`
- Добавить `kKeywordClock` в enum `TokenType`

### T3.2 — Лексер
**Файл:** `src/live/dsl/Lexer.cpp`
- В карту ключевых слов добавить `{"clock", TokenType::kKeywordClock}`

### T3.3 — Парсер
**Файл:** `src/live/dsl/Parser.cpp`
- Создать функцию `parseClockBlock(ParserState& state, ClockBlock& clock, diagnostics)`:
  - Ожидает `{`
  - Внутри парсит свойства: `enabled = <expression>`, `z = <expression>`, `blend = <identifier>`, `alpha = <expression>`
  - Все свойства опциональны
  - Ожидает `}`
- В `parseProgram()`: после парсинга `effect "name"` и перед циклом `while` — проверка `kKeywordClock`:
  ```cpp
  if (state.current().type == TokenType::kKeywordClock) {
    state.match(TokenType::kKeywordClock);
    if (!parseClockBlock(state, parsedProgram.clockBlock, diagnostics)) {
      return false;
    }
    skipNewlines(state);
  }
  ```
- В `parseLayer()`: добавить `case TokenType::kKeywordAlpha:` для парсинга `alpha = <expression>` в свойствах слоя (опционально — если решим дать alpha и обычным слоям)

### T3.4 — Подсветка синтаксиса
**Файл:** `frontend/src/editor/luxHighlight.ts`
- Добавить `"clock"` в массив ключевых слов

---

## T4: Компиляция `clock { ... }` блока

**Файл:** `src/live/runtime/Compiler.cpp`

### T4.1 — В `Compiler::compile()`:
- После `compiled.effectName = program.effectName;`:
  ```cpp
  // Compile clock block
  if (program.clockBlock.hasBlock) {
    compiled.clockConfig.enabled = true;  // default
    if (!program.clockBlock.enabledExpression.empty()) {
      // Compile enabled as expression; if constant 0 → enabled=false
      // Для простоты: если expression = "0", enabled = false
      // Можно скомпилировать в expression node и вычислять в runtime
      int16_t enabledExpr = -1;
      expressionCompiler.compile(program.clockBlock.enabledExpression, enabledExpr, diagnostics, program.clockBlock.enabledLine);
      // Сохраняем выражение или вычисляем константу
      // ... (упрощённо: проверяем является ли константой 0)
    }
    if (!program.clockBlock.zExpression.empty()) {
      expressionCompiler.compile(program.clockBlock.zExpression, compiled.clockConfig.zExpression, diagnostics, program.clockBlock.zLine);
    }
    if (!program.clockBlock.blendMode.empty()) {
      compileBlendMode(program.clockBlock.blendMode, compiled.clockConfig.blendMode, diagnostics, program.clockBlock.blendLine);
    }
    if (!program.clockBlock.alphaExpression.empty()) {
      expressionCompiler.compile(program.clockBlock.alphaExpression, compiled.clockConfig.alphaExpression, diagnostics, program.clockBlock.alphaLine);
    }
  }
  ```

**Примечание:** Поле `enabled` — особый случай. В отличие от других полей, это не expression для per-pixel вычисления, а флаг уровня эффекта. Если `enabled = 0`, часы не рендерятся вообще. Можно:
- Вариант A: Скомпилировать как expression и проверять в runtime (гибко: `enabled = t > 5`)
- Вариант B: Только константы 0/1 (проще, достаточно для текущих нужд)

**Решение (agent discretion):** Вариант B — только константы `0`/`1`. Если не константа — diagnostic warning, считаем enabled=true.

---

## T5: Z-сортировка в Executor

**Файл:** `src/live/runtime/Executor.cpp`

### T5.1 — Рефакторинг `Executor::render()`

Текущая логика: цикл по `program.layers` → рендер каждого слоя → попутно `lastRenderOnTop_`.

Новая логика:
1. Собрать все сущности для рендера в один список:
   - Каждый DSL-слой → `RenderItem{z = eval(zExpression), type = Layer, index}`
   - Часы (если `clockConfig.enabled && clockVisible`) → `RenderItem{z = eval(clockConfig.zExpression, default=1), type = Clock}`
2. Отсортировать по `z` (stable sort — при равных z порядок сохраняется)
3. Пройти по отсортированному списку и отрендерить каждый элемент:
   - `type == Layer` → текущая логика рендера слоя
   - `type == Clock` → вызвать рендер часов с blend/alpha из `clockConfig`

### T5.2 — Удалить `lastRenderOnTop_`
- Убрать поле `lastRenderOnTop_` из `Executor.h`
- Убрать `lastRenderOnTop_ = false;` и `lastRenderOnTop_ = true;` из `render()`
- Убрать `isEffectOnTop()` из `LiveProgramService.h` и `LiveProgramService.cpp`

### T5.3 — Рендер часов внутри Executor
- `Executor::render()` принимает callback или прямой вызов для рендера часов
- **Вариант A:** Executor получает указатель на `ClockOverlay` и вызывает его
- **Вариант B:** `main.cpp` передаёт в `ExecutionContext` готовый `FrameBuffer`-слой с часами
- **Решение:** Вариант A — добавляем `ClockOverlay*` в `ExecutionContext` (или функцию-callback). Executor сам вызывает `clockOverlay->render(...)` с учётом blend/alpha из `clockConfig`.

### T5.4 — Alpha-смешивание
- В `renderSpritePixel()` и месте рендера часов с `hasPixelColor`: после `blendColors()` применить alpha:
  ```cpp
  lamp::Rgb blended = blendColors(layer.blendMode, dst, src);
  float alpha = /* layer alpha или clock alpha */;
  if (alpha < 1.0f) {
    blended = lamp::Rgb{
      static_cast<uint8_t>(blended.r * alpha + dst.r * (1.0f - alpha)),
      static_cast<uint8_t>(blended.g * alpha + dst.g * (1.0f - alpha)),
      static_cast<uint8_t>(blended.b * alpha + dst.b * (1.0f - alpha))
    };
  }
  frameBuffer.setPixel(physX, physY, blended);
  ```
- Для DSL-слоёв: alpha пока не добавляем в этой фазе (только для часов). Поле `alphaExpression` в `CompiledLayer` можно добавить позже.
- Для часов: использовать `clockConfig.alphaExpression` (default = 1.0).

---

## T6: ClockOverlay → blend + alpha

**Файлы:** `src/effects/ClockOverlay.cpp`, `include/effects/ClockOverlay.h`

### T6.1 — Изменить сигнатуру `render()`
- Добавить параметры `BlendMode blendMode` и `float alpha`
- Или: передавать `CompiledClockConfig`
```cpp
void render(const std::string& currentTime, lamp::FrameBuffer& frameBuffer,
            bool visible, uint32_t nowMs,
            float temperatureC, float humidityPercent, bool sensorAvailable,
            BlendMode blendMode = BlendMode::kNormal, float alpha = 1.0f) const;
```

### T6.2 — Использовать blendColors + alpha
- Вместо `fb.setPixel(x, y, color)`:
  ```cpp
  lamp::Rgb dst = fb.getPixel(x, y);
  lamp::Rgb blended = blendColors(blendMode, dst, color);
  if (alpha < 1.0f) {
    blended = lerpColor(dst, blended, alpha);
  }
  fb.setPixel(x, y, blended);
  ```
- Вынести `blendColors()` в общий хедер (например `include/BlendUtils.h`) или оставить в Executor и добавить include.

### T6.3 — Вынести blendColors в общий модуль
**Новый файл:** `include/BlendUtils.h`
```cpp
#pragma once
#include "FrameBuffer.h"
#include "live/runtime/CompiledProgram.h"

namespace lamp {
inline Rgb blendColors(BlendMode mode, Rgb dst, Rgb src) { /* ... */ }
inline Rgb lerpColor(Rgb a, Rgb b, float t) { /* ... */ }
}
```
Убрать `blendColors` из анонимного namespace в `Executor.cpp`, заменить на `lamp::blendColors`.

---

## T7: main.cpp — убрать `isEffectOnTop()`

**Файл:** `src/main.cpp`

### T7.1 — Рефакторинг `renderOverlayPass()`
- Убрать `if (g_liveProgramService.isEffectOnTop()) return;`
- Вместо этого: часы рендерятся внутри `Executor::render()` через z-сортировку
- `renderOverlayPass()` → вызывается только для compiled-эффектов (когда live не активен)
- Для compiled-эффектов: часы рендерятся поверх (старое поведение)

### T7.2 — Передача ClockOverlay в Executor
- В `LiveProgramService::render()`: пробросить `g_clockOverlay` и `g_runtimeTimeState` в `ExecutionContext`
- `ExecutionContext` расширяется:
  ```cpp
  struct ExecutionContext {
    float timeSeconds, deltaSeconds, temperatureC, humidityPercent;
    CompiledClockConfig clockConfig;
    // Для рендера часов:
    const effects::ClockOverlay* clockOverlay = nullptr;
    std::string currentTime;
    bool clockVisible = false;
    uint32_t nowMs = 0;
    bool sensorAvailable = false;
  };
  ```

### T7.3 — Обновить `renderFrame()`
- `renderEffectPass()` теперь включает и эффект, и часы (для live-эффектов)
- `renderOverlayPass()` — только для compiled-эффектов

---

## T8: Frontend — справка и подсветка

**Файл:** `frontend/src/editor/help.ts`

### T8.1 — Разгруппировать rotation / blend
- Удалить: `{ term: "rotation / blend", description: "Вращение и режим наложения (normal, add, multiply, screen)." }`
- Добавить:
  ```typescript
  { term: "rotation", description: "Вращение спрайта в радианах. Пример: rotation = t * 1.5" },
  { term: "blend", description: "Режим наложения: normal, add, multiply, screen." },
  ```

### T8.2 — Проверить luxHighlight.ts
- `screen` уже есть в массиве (строка 19). Оставить.
- Добавить `"clock"` в массив ключевых слов.

---

## T9: Обновить тесты

### T9.1 — test_dsl_parser
**Файлы:** `test/test_dsl_parser/`
- Добавить тест-кейс на парсинг `clock { enabled = 0, z = 5, blend = add, alpha = 0.5 }`
- Добавить тест-кейс на парсинг пустого `clock { }` (все default)
- Добавить тест-кейс на `clock { }` в эффекте без effect (ошибка)

### T9.2 — test_dsl_executor
**Файлы:** `test/test_dsl_executor/`
- Добавить тест на `screen` blend mode
- Добавить тест на z-сортировку: слой z=0 → слой z=2 → слой z=1 → проверка порядка рендера
- Добавить тест на `clock { enabled = 0 }` — часы не рендерятся

### T9.3 — test_live_api
**Файлы:** `test/test_live_api/`
- Добавить тест на валидацию DSL с `clock { ... }` блоком

---

## T10: Обновить документацию

**Файлы:** `docs/DSL.md`, `docs/LLM_EFFECT_PROMPT.md`

### T10.1 — DSL.md
- Добавить секцию «clock» после «effect»:
  ```markdown
  ## `clock`
  Блок `clock { ... }` управляет отображением часов в эффекте.
  Все поля опциональны.
  Поля: `enabled` (0/1, default 1), `z` (позиция, default 1),
  `blend` (normal/add/multiply/screen, default normal),
  `alpha` (0.0–1.0, default 1.0).
  ```
- Обновить секцию про `blend`: добавить `screen`
- Обновить секцию про `layer`: документировать `z`

### T10.2 — LLM_EFFECT_PROMPT.md
- Добавить в примеры использование `clock { enabled = 0 }` для эффектов где часы не нужны
- Добавить пример с `clock { z = 999, blend = add, alpha = 0.5 }`

---

## Порядок выполнения

```
Параллельно: T1 (screen) + T2 (structs) + T3 (parser)
    ↓
T4 (compiler) — зависит от T2, T3
    ↓
T5 (z-sorting) + T6 (clock blend) — зависит от T1, T2
    ↓
T7 (main.cpp cleanup) — зависит от T5
    ↓
T8 (frontend) — зависит от T1
    ↓
T9 (tests) — зависит от T5, T6
    ↓
T10 (docs) — после всех
```

---

## Контрольные точки

| После | Что проверяем |
|--------|---------------|
| T1 | `blend = screen` работает в DSL-эффекте (визуально осветляет) |
| T3 | `clock { enabled = 0 }` парсится без ошибок |
| T4 | `clock { blend = add, alpha = 0.5 }` компилируется |
| T5 | Слои с z=0 рендерятся ПОД часами, z=2 — НАД |
| T6 | Часы с `blend = add` светятся, с `alpha = 0.5` полупрозрачны |
| T7 | `isEffectOnTop()` удалён, старые эффекты работают без изменений |
| T8 | В справке rotation и blend — отдельные пункты |
| T9 | Все тесты проходят: `platformio test -e native` |
| T10 | DSL.md и LLM_EFFECT_PROMPT.md актуальны |

---

*Plan created: 2026-06-03*
