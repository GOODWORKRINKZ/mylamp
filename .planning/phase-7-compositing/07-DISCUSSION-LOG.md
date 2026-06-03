# Phase 7: Layer Compositing & Blend - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-06-03
**Phase:** 7-layer-compositing-blend
**Areas discussed:** Z-ordering, Clock compositing, DSL clock control, Blend modes, Help/docs, Layer compositing

---

## Z-ordering

| Option | Description | Selected |
|--------|-------------|----------|
| Трёхфазный рендер | Фаза 1: z=0 → Фаза 2: часы → Фаза 3: z≥1 | |
| Z-сортировка всех слоёв | Слои и часы в одной куче, сортировка по z | ✓ |
| Два буфера | Эффект в FB1, часы в FB2, мерж | |

**User's choice:** Z-сортировка всех слоёв — часы как обычный слой с z.
**Notes:** Пользователь хочет гибкость: в for-эффектах z = индекс, часы на z=999.

| Option | Description | Selected |
|--------|-------------|----------|
| z = 0 у часов | Слои z<0 под, z=0 вперемешку, z>0 над | |
| z = 0.5 у часов | z=0 под, z=1 над — обратная совместимость | |
| Настраиваемый z | В DSL задаётся z часов | ✓ |

**User's choice:** Настраиваемый z через DSL `clock { z = 999 }`. Default = 1.

| Option | Description | Selected |
|--------|-------------|----------|
| Наследует z часов | Сенсоры — часть ClockOverlay | ✓ |
| Отдельный z | z сенсоров отдельно от часов | |

**User's choice:** Наследует z часов.

---

## Clock compositing

| Option | Description | Selected |
|--------|-------------|----------|
| Normal (замена) | Часы заменяют пиксели эффекта | |
| Add (свечение) | Часы добавляются к эффекту | |
| Настраиваемый blend | В DSL: clock_blend = add. Default = normal | ✓ |

**User's choice:** Часы поддерживают те же blend-режимы что и слои. «также как и для слоев!»

| Option | Description | Selected |
|--------|-------------|----------|
| Нет, не сейчас | Пустые места между цифр уже дают прозрачность | |
| Да, alpha в DSL | clock_alpha = 0.7 — полупрозрачные часы | ✓ |

**User's choice:** Добавить alpha. «можно и альфаканал ему навертеть»

---

## DSL: clock control

| Option | Description | Selected |
|--------|-------------|----------|
| clock = off | Краткая форма | |
| clock { enabled = 0 } | Блок настроек | |
| Оба варианта | И shorthand, и блок | |

**User's choice:** `clock = 0` как shorthand. Но в итоге уточнено до блока `clock { enabled = 0, z = ..., blend = ..., alpha = ... }`.

| Option | Description | Selected |
|--------|-------------|----------|
| Отдельными полями | clock_z, clock_blend, clock_alpha | |
| Блоком clock { } | Структурно, группирует настройки | ✓ |

**User's choice:** Блок `clock { ... }`.

---

## Blend modes

| Option | Description | Selected |
|--------|-------------|----------|
| + screen | 4 режима: normal, add, multiply, screen | |
| + screen, overlay | 5 режимов | |
| Ты реши | Агент выбирает разумный набор | ✓ |

**User's choice:** «Ты реши». Агент выбрал: normal, add, multiply, screen.

| Option | Description | Selected |
|--------|-------------|----------|
| Документировать | В справке: multiply требует нечёрный фон | ✓ |
| Фоновый слой | background = rgb(...) в DSL | |

**User's choice:** Документировать. Фоновый слой отложен.

---

## Help/docs

| Option | Description | Selected |
|--------|-------------|----------|
| Разделить + убрать screen | rotation и blend — отдельно. screen убран до реализации | ✓ |
| Разделить + реализовать screen | Сначала реализовать, потом обновить | |

**User's choice:** Разделить rotation/blend, screen временно убрать из справки.

---

## Layer compositing (между собой)

| Option | Description | Selected |
|--------|-------------|----------|
| Да, всё верно | Painter's algorithm — стандарт. Документировать | ✓ |
| Нет, хочу другое | Альтернативное поведение | |

**User's choice:** Painter's algorithm — правильное поведение.

---

## the agent's Discretion

- Набор blend-режимов: normal, add, multiply, screen (выбрано агентом)
- Конкретная реализация z-сортировки
- Структура ClockConfig
- Порядок обновления файлов (firmware → frontend)

## Deferred Ideas

- `background = rgb(...)` в DSL для нечёрного фона — нужно для multiply
- Другие overlay как отдельные слои (индикаторы статуса)
- Дополнительные blend-режимы: overlay, difference, darken, lighten
- Настраиваемый шрифт/цвет часов
