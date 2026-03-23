# Documentation Structure Redesign Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Пересобрать документацию так, чтобы README был короткой и сильной входной точкой, а детали DSL и статуса проекта были вынесены в отдельные документы.

**Architecture:** README становится индексом и quick-start документом. Спецификация языка и честный статус продукта выносятся в отдельные markdown-файлы, связанные явной навигацией.

**Tech Stack:** Markdown, существующая структура `docs/`, текущие verified данные из firmware и frontend.

---

### Task 1: Rebuild README as the entry point

**Files:**
- Modify: `README.md`

**Step 1: Replace the long mixed structure**

Перестроить README в такой порядок:
- заголовок и короткое описание проекта;
- блок «Что это»;
- блок «Что уже работает»;
- блок «Быстрый старт»;
- блок «Железо и подключение»;
- блок «Как делать эффекты»;
- блок «Куда читать дальше».

**Step 2: Add explicit navigation**

Вставить явные ссылки на:
- `docs/DSL.md`
- `docs/STATUS.md`
- `docs/LLM_EFFECT_PROMPT.md`
- `docs/ARCHITECTURE.md`

**Step 3: Preserve only high-value detail**

Оставить в README только те детали, которые помогают стартовать без чтения остальных файлов. Всё, что относится к полной спецификации DSL и к честному списку ограничений, вынести.

### Task 2: Create DSL reference document

**Files:**
- Create: `docs/DSL.md`

**Step 1: Move syntax contract into a dedicated file**

Добавить разделы:
- назначение DSL;
- сущности `effect`, `sprite`, `layer`;
- разрешённые поля;
- переменные;
- функции;
- ограничения v1.

**Step 2: Add examples**

Включить:
- минимальный шаблон эффекта;
- один более живой пример;
- короткий блок про хорошую постановку задачи для LLM.

### Task 3: Create verified status document

**Files:**
- Create: `docs/STATUS.md`

**Step 1: Write the current capability snapshot**

Разделить состояние на:
- готово;
- частично готово;
- ещё не реализовано.

**Step 2: Call out critical limitation clearly**

Зафиксировать, что frontend и mock backend поддерживают live workflow, но device-side `POST /api/live/validate` и `POST /api/live/run` ещё не доведены до финальной реализации.

### Task 4: Verify documentation coherence

**Files:**
- Check: `README.md`
- Check: `docs/DSL.md`
- Check: `docs/STATUS.md`

**Step 1: Verify link targets**

Проверить, что все файлы, на которые ссылается README, реально существуют.

**Step 2: Verify consistency of claims**

Сверить, что README не обещает больше, чем написано в `docs/STATUS.md`.

**Step 3: Final readability pass**

Убрать повторы и сделать секции короткими и сканируемыми.