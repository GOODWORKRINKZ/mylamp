# Phase 7: Render Pipeline — Layer Compositing & Blend - Context

**Gathered:** 2026-06-03
**Status:** Ready for planning

<domain>
## Phase Boundary

Починить и расширить композитинг слоёв в render pipeline:

- **Z-ordering**: заменить бинарный флаг (часы либо поверх всего, либо скрыты) на настоящую z-сортировку слоёв и часов
- **Clock as layer**: часы становятся полноценным слоем с blend, alpha, z — как обычные DSL-слои
- **DSL clock control**: новый блок `clock { ... }` в DSL для управления часами из эффекта
- **Blend modes**: добавить `screen`, документировать `multiply`, поправить справку редактора
- **Help/docs**: разгруппировать `rotation`/`blend` в подсказке, синхронизировать справку с реальностью

</domain>

<decisions>
## Implementation Decisions

### Z-ordering
- **D-01:** Z-сортировка всех слоёв + часов в одном списке. Рендер по возрастанию z. Слои без z → default z = 0.
- **D-02:** Часы — такой же слой с настраиваемым z. Default z часов = 1 (слои с z=0 под часами, z≥2 над — обратная совместимость со старыми эффектами без z).
- **D-03:** Сенсорная строка (температура/влажность) наследует z часов — рендерится вместе с ними как часть ClockOverlay.
- **D-04:** DSL-блок `clock { ... }` позволяет переопределить z: `clock { z = 999 }`.

### Clock compositing
- **D-05:** Часы поддерживают те же blend-режимы, что и слои: `normal` (default), `add`, `multiply`, `screen`.
- **D-06:** Часы поддерживают alpha: `clock { alpha = 0.7 }`. Default alpha = 1.0. Реализуется через смешивание цвета часов с пикселем фреймбуфера: `dst = blend(mode, dst, src) * alpha + dst * (1 - alpha)`.
- **D-07:** ClockOverlay переделывается с прямого `setPixel()` на использование `blendColors()` из Executor (или общей утилиты).

### DSL: clock control
- **D-08:** Новый блок `clock { ... }` на уровне эффекта (после `effect "name"`, до sprite/layer):
  ```
  clock {
    enabled = 1    // 0 — часы скрыты, 1 — показаны (default)
    z = 1          // z-позиция (default = 1)
    blend = normal // режим наложения (default = normal)
    alpha = 1.0    // прозрачность (default = 1.0)
  }
  ```
- **D-09:** Все поля блока опциональны — неуказанные берут default.
- **D-10:** Если блок `clock` отсутствует — часы работают с глобальными настройками и default-параметрами.
- **D-11:** Парсер, компилятор, executor поддерживают новый блок. `LiveProgramService` пробрасывает настройки часов в `ExecutionContext`.

### Blend modes
- **D-12:** Набор blend-режимов: `normal`, `add`, `multiply`, `screen`. (Выбрано агентом — минимальный полезный набор для ESP32-C3.)
- **D-13:** `screen`: формула `255 - ((255-dst) * (255-src)) / 255`. Инвертированный multiply — осветляет, хорош для свечения и частиц.
- **D-14:** `multiply` остаётся как есть. В справку добавляется примечание: «multiply требует нечёрный фон (умножение на чёрный даёт чёрный)».

### Layer compositing (между собой)
- **D-15:** Слои композитятся по painter's algorithm: слой 0 → результат → слой 1 поверх → результат → слой 2 поверх и т.д. Каждый слой смешивается с накопленным результатом через свой `blend`. Это правильное поведение — документировать в справке.

### Справка редактора
- **D-16:** Разделить `rotation / blend` на два отдельных пункта в `help.ts`:
  - `rotation` — «Вращение спрайта в радианах: rotation = t * 1.5»
  - `blend` — «Режим наложения: normal, add, multiply, screen»
- **D-17:** Убрать `screen` из `luxHighlight.ts` (ключевые слова) до реализации, затем вернуть.
- **D-18:** После реализации — обновить `luxHighlight.ts` и `help.ts` с актуальным списком blend-режимов.

### the agent's Discretion
- Конкретная реализация z-сортировки (std::sort по z-expression, или отдельные фазы)
- Структура `ClockConfig` в compiled program / execution context
- Точный синтаксис парсинга блока `clock { ... }` (как расширение парсера effect-заголовка)
- Порядок обновления файлов: сначала firmware (blend, z-sorting, clock block), потом frontend (help fix)

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Render pipeline
- `src/main.cpp` — `renderEffectPass()`, `renderOverlayPass()`, `renderFrame()` — текущий render loop
- `src/live/runtime/Executor.cpp` — `Executor::render()`, `blendColors()`, `renderSpritePixel()` — рендер DSL-слоёв и blend
- `src/effects/ClockOverlay.cpp` — `ClockOverlay::render()` — текущая реализация часов (прямой `setPixel`)
- `include/effects/ClockOverlay.h` — интерфейс ClockOverlay

### DSL pipeline
- `include/live/dsl/Ast.h` — `LayerDeclaration`, `Program` — AST структуры (добавить `ClockBlock`)
- `include/live/dsl/Token.h` — токены лексера (добавить `kKeywordClock`)
- `src/live/dsl/Parser.cpp` — парсер (добавить парсинг `clock { ... }`)
- `src/live/dsl/Lexer.cpp` — лексер (добавить ключевое слово `clock`)
- `include/live/runtime/CompiledProgram.h` — `CompiledLayer`, `CompiledProgram`, `BlendMode` (добавить `kScreen`, `CompiledClockConfig`)
- `src/live/runtime/Compiler.cpp` — компилятор AST → CompiledProgram (добавить компиляцию `clock` блока, `screen`)
- `include/live/runtime/Executor.h` — `ExecutionContext`, `lastRenderOnTop_`

### Runtime services
- `include/live/runtime/LiveProgramService.h` — `isEffectOnTop()`, `render()`
- `include/live/runtime/RuntimeContext.h` — `RuntimeContext` (добавить clock config)

### Frontend
- `frontend/src/editor/help.ts` — `editorHelpSections` — справка редактора (разгруппировать rotation/blend)
- `frontend/src/editor/luxHighlight.ts` — ключевые слова для подсветки (screen, blend-режимы)

### Specs
- `docs/DSL.md` — референс языка Lux (обновить с блоком `clock { ... }` и blend-режимами)
- `docs/LLM_EFFECT_PROMPT.md` — промпт для LLM-генерации (обновить с новыми возможностями)

### Hardware
- `include/AppConfig.h` — kLogicalWidth=32, kLogicalHeight=16
- `include/FrameBuffer.h` — `getPixel()`, `setPixel()` — основа для blend

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- **`blendColors()`** в `Executor.cpp` — уже реализована для 3 режимов. Можно вынести в общую утилиту (`BlendUtils.h`) и использовать и в ClockOverlay, и в Executor. Добавить `kScreen`.
- **`CompiledLayer`** — уже имеет `blendMode`, `zExpression`. Структура готова к сортировке.
- **DSL pipeline** (`Lexer → Parser → Compiler → Executor`) — конвейер уже расширялся для `for`, `frame`, `rotation`. Добавление `clock { ... }` и `screen` идёт по тому же шаблону.
- **`ExecutionContext`** — уже содержит `timeSeconds`, `deltaSeconds`, `temperatureC`, `humidityPercent`. Добавить `clockConfig`.
- **`LiveProgramService::render()`** — точка интеграции, где можно передать clock config в executor.

### Established Patterns
- **Planner-Service**: `TimePlanner`/`TimeRuntimeService` — часы уже управляются через этот паттерн. `clock { enabled = 0 }` в DSL должен координироваться с глобальным `clock.enabled`.
- **`Diagnostic` + русские сообщения**: все ошибки парсинга/компиляции DSL на русском.
- **Property-парсинг**: `rotation`, `blend`, `visible`, `frame` — все парсятся однотипно в `Parser.cpp`. `clock { ... }` — новый вид блока на уровне effect.
- **Expression evaluation**: `evaluateNode()` используется для всех expression-полей слоя. Z-значения тоже должны вычисляться через неё.

### Integration Points
- **`renderFrame()` → `renderEffectPass()` → `renderOverlayPass()`**: нужно заменить на единый проход с z-сортировкой. `renderEffectPass` рендерит DSL-слои в framebuffer, `renderOverlayPass` — часы. После изменений: собрать все слои (DSL + clock), отсортировать по z, отрендерить.
- **`LiveProgramService::isEffectOnTop()`**: уходит. Заменяется на z-сортировку.
- **`ClockOverlay::render()`**: нужно переделать с `setPixel()` на `blendColors()` + alpha.
- **`g_timeState.clockOverlayVisible`**: глобальный флаг видимости часов. Должен учитывать `clock { enabled = 0 }` из DSL.

### Что сломается при изменениях
- **Обратная совместимость DSL**: старые эффекты без `z` и без `clock { ... }` должны работать как раньше (слои под часами, часы visible).
- **`lastRenderOnTop_`**: удалить. Вся логика переносится в z-сортировку.
- **Тесты**: `test_live_api`, `test_dsl_executor`, `test_dsl_parser` — нужно обновить под новый `clock` блок и `screen`.

</code_context>

<specifics>
## Specific Ideas

- Пользователь хочет делать эффекты с `for`-циклом, где `z = индекс_цикла`, и управлять положением часов среди этих слоёв через `clock { z = 999 }`.
- `clock = 0` рассматривался как краткая форма, но выбран блок `clock { enabled = 0 }` для единообразия.
- `screen` уже обещан в подсказке редактора — нужно реализовать, а не убирать навсегда.

</specifics>

<deferred>
## Deferred Ideas

- **Фоновый слой эффекта**: `background = rgb(10, 10, 30)` в DSL — чтобы `multiply` работал на нечёрном фоне. Обсуждалось, но отложено.
- **Другие overlay**: индикаторы статуса, сенсоры как отдельные слои — не в этом скоупе.
- **Дополнительные blend-режимы**: `overlay`, `difference`, `darken`, `lighten` — можно добавить позже если будет запрос.
- **Настраиваемый шрифт/цвет часов**: пока часы используют фиксированный белый цвет и 3×5 шрифт.

</deferred>

---

*Phase: 7-Layer-Compositing-Blend*
*Context gathered: 2026-06-03*
