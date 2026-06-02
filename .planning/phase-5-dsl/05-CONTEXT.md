# Phase 5: Demo Effects & DSL - Context

**Gathered:** 2026-06-02
**Status:** Ready for planning

<domain>
## Phase Boundary

Добавить крутые демо-эффекты с анимацией спрайтов на цилиндрической LED-матрице.
Расширить Lux DSL ровно настолько, насколько нужно для реализации этих эффектов.
Починить сохранение пресетов из редактора.
Добавить кнопку «Поделиться» (base64-ссылка).

</domain>

<decisions>
## Implementation Decisions

### Модель анимации
- **D-01:** Именованные кадры в `sprite`. Синтаксис: `sprite name { frame walk1 { bitmap "..." } frame walk2 { bitmap "..." } }`. Каждый кадр — отдельный bitmap внутри sprite.
- **D-02:** Выражение для выбора кадра в `layer`: поле `frame = <expression>` (целочисленное). Например: `frame = (t * 4) % 3` для цикла из 3 кадров.

### Расширения DSL
- **D-03:** Циклы `for`: синтаксис `for <var> = <start>; <var> < <end>; <var> = <var> + <step> { ... }`. Переменная цикла видна только внутри тела. Нужны для fullscreen-эффектов без ручного рисования bitmap 32×16.
- **D-04:** Обратная совместимость: все существующие `.lux`-эффекты должны работать без изменений. Старый `sprite` без `frame` = один кадр по умолчанию. Старый `layer` без `frame` = кадр 0.
- **D-05:** `layer.use` остаётся литералом имени спрайта. Выбор кадра — через новое поле `frame`, не через `use`.

### Демо-эффекты
- **D-06:** Обязательный набор: Nyan Cat (котик с радужным шлейфом), Mario (бегущий по кругу), Плазма/Perlin noise (fullscreen-абстракция), Бегущая строка (текст по окружности), Змейка (Snake на цилиндре).
- **D-07:** Дополнительно 2–3 эффекта на выбор разработчика из пула: Огонь/частицы вверх, Звёздное поле, ДНК-спираль, DVD-логотип.
- **D-08:** Все демо-эффекты пишутся на Lux DSL (не C++ IEffect). Это живая демонстрация возможностей языка.

### Доставка демо-эффектов
- **D-09:** Демки доступны и как starter-сниппеты в редакторе (кнопка «Идеи»), и как предустановленные пресеты на устройстве (при первой прошивке).
- **D-10:** Кнопка «Поделиться» копирует DSL-код эффекта, закодированный в base64-ссылку. Принимающая сторона вставляет ссылку — редактор декодирует и открывает эффект.

### Сохранение пресетов
- **D-11:** Починить сохранение пресета из редактора. Сейчас `runTemporary` работает, а сохранение — нет. Кнопка «Сохранить» должна создавать/обновлять пресет через `PresetRepository::save()`.

### the agent's Discretion
- Конкретный синтаксис `for`-цикла (точный набор операторов сравнения, поддержка `<=` / `>=`, вложенные циклы)
- Порядок кадров в sprite (индексный внутри, имена — для пользователя)
- Формат base64-ссылки (чистый base64 или URL-safe base64, с префиксом или без)
- Выбор дополнительных 2–3 эффектов из пула
- Точный пиксель-арт для спрайтов (Марио, котик) — адаптировать под 32×16 цилиндр

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### DSL & Language
- `docs/DSL.md` — Полный референс языка Lux: синтаксис, сущности (effect/sprite/text/layer), переменные, функции, ограничения
- `docs/LLM_EFFECT_PROMPT.md` — Рекомендации для LLM-генерации эффектов

### Current implementation
- `include/live/dsl/Ast.h` — AST-структуры: SpriteDeclaration, LayerDeclaration, Program
- `include/live/dsl/Token.h` — Токены лексера (ключевые слова DSL)
- `include/live/runtime/CompiledProgram.h` — Байткод: CompiledSprite, CompiledLayer, ExpressionNode/ExpressionOp, BlendMode
- `include/live/runtime/Compiler.h` — Компилятор AST → CompiledProgram
- `include/live/runtime/Executor.h` — Исполнитель байткода (ExecutionContext с timeSeconds, deltaSeconds, temperatureC, humidityPercent)
- `include/live/runtime/LiveProgramService.h` — Оркестрация: validateSource, runTemporary, activatePreset, render
- `include/live/PresetModel.h` — PresetModel (id, name, source, tags, options)
- `include/live/PresetRepository.h` — CRUD пресетов на LittleFS

### Hardware constraints
- `include/AppConfig.h` — kLogicalWidth=32, kLogicalHeight=16, параметры панелей
- `include/FrameBuffer.h` — fillRect, drawLine, drawCircle с горизонтальным wraparound
- `include/MatrixLayout.h` — Логические → физические индексы LED

### Frontend
- `frontend/src/editor/snippets.ts` — Существующие starter-сниппеты (rainbow, fire, upside-down, heart, lightning)
- `frontend/src/editor/help.ts` — Справочник по ключевым словам DSL в редакторе
- `frontend/src/editor/luxHighlight.ts` — Подсветка синтаксиса (ключевые слова)

### Reference
- [Nyan Cat pixel art reference](https://encrypted-tbn0.gstatic.com/images?q=tbn:ANd9GcQ0M95o199E58oG5w2NrKrSRRpgZ3sUecLCuS74SVf44QhCL3MxtTg52nYIOYFEYgFs4lvwmPAJsuXdj8LUCQ5EacE0xutJ3rB_kXVqHS45Mg) — референс для пиксель-арта котика

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- **DSL pipeline** (`Lexer → Parser → Compiler → Executor`): расширяется новыми ключевыми словами (`for`, `frame`) и узлами выражений
- **`CompiledProgram`**: структура `CompiledSprite` уже хранит массив пикселей — можно расширить до массива кадров (`std::vector<std::vector<CompiledSpritePixel>> frames`)
- **`ExpressionNode`**: древовидная структура с `children[4]` — циклы и frame-выражения добавляют новые `ExpressionOp`
- **`StarterSnippets`**: массив `starterSnippets` в `frontend/src/editor/snippets.ts` — добавляем новые демки сюда
- **`PresetRepository` + `LittleFsFileStore`**: CRUD пресетов работает, нужно починить сохранение из API
- **`LiveProgramService::validateSource` / `runTemporary`**: работают — значить проблема сохранения в web-слое или API-эндпоинте

### Established Patterns
- **Planner-Service**: эффекты не должны нарушать этот паттерн — DSL-демки исполняются через `LiveProgramService`
- **`Diagnostic` + русские сообщения**: все ошибки DSL возвращаются с русским текстом для фронтенда
- **Per-pixel выражения**: `nx`, `ny`, `x`, `y` доступны в выражениях цвета — основа для fullscreen-эффектов (Изнанка)
- **Спрайты через bitmap**: `#` = пиксель, `.` = пусто — сохраняем этот синтаксис для кадров

### Integration Points
- **`LampWebServer`**: добавить API-эндпоинт для сохранения пресета (если ещё нет) и эндпоинт для экспорта/импорта
- **`main.cpp`**: предпосев пресетов при первом старте (проверка LittleFS на наличие factory-пресетов)
- **Фронтенд `main.ts`**: кнопка «Поделиться» + кнопка «Сохранить» + новые сниппеты в редакторе
- **`luxHighlight.ts`**: добавить `for`, `frame` в список ключевых слов подсветки
- **`help.ts`**: добавить справку по новым конструкциям (`for`, `frame`)

</code_context>

<specifics>
## Specific Ideas

- **Nyan Cat**: котик с Pop-Tart телом, розовой глазурью, бегущими ножками. Радужный шлейф позади — через fullscreen-слой с per-pixel выражением на основе `smoothstep` и дистанции до котика. Предоставлен пиксель-арт референс.
- **Mario**: 8-bit Марио, бегущий по окружности цилиндра. 2–4 кадра анимации ходьбы.
- **Плазма/Perlin**: fullscreen-эффект без спрайта — чистые per-pixel вычисления с `sin`/`cos`/`nx`/`ny`. Показать мощь DSL.
- **Бегущая строка**: текст, движущийся по окружности. Использует встроенный шрифт 3×5 + `text`-конструкцию + `for` для посимвольного рендеринга.
- **Змейка**: классическая Snake — использует `for` для отрисовки сегментов. Показывает, что на DSL можно писать игровую логику.
- **Кнопка «Поделиться»**: кодирует DSL-код в base64 (url-safe), копирует в буфер обмена. При вставке в редактор — декодирует. Компактно для чатов/мессенджеров.
- **Текущие эффекты кроме Изнанки — слабые**: радуга, огонь, сердечко, молния воспринимаются как невпечатляющие. Новые демки должны быть visually striking на цилиндре.

</specifics>

<deferred>
## Deferred Ideas

- **Автосохранение истории**: все временно запущенные эффекты попадают в историю — отдельная фича, не в этой фазе
- **Превью/тамбнейлы эффектов**: картинки-превью для пресетов — нескоро, требует рендер-фермы или экранных скриншотов
- **Кастомные шрифты** (пользовательские, не встроенный 3×5): не в этой фазе, встроенного шрифта достаточно для демо
- **Массивы/таблицы данных в DSL**: не нужны для текущих демок, отложено до реальной потребности

</deferred>

---

*Phase: 5-dsl*
*Context gathered: 2026-06-02*
