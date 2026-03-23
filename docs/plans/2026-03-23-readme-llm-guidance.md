# README and LLM Guidance Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Обновить README так, чтобы он стал основной документацией по подключению, запуску, DSL и LLM-guidance для генерации эффектов.

**Architecture:** Работа ограничена документацией. Основной артефакт — README.md. Дополнительно сохраняется дизайн-док и этот план, чтобы структура изменений и границы утверждений были прозрачны.

**Tech Stack:** Markdown, существующая документация проекта, текущий frontend DSL contract.

---

### Task 1: Gather factual project details

**Files:**
- Read: `README.md`
- Read: `include/AppConfig.h`
- Read: `docs/ARCHITECTURE.md`
- Read: `frontend/src/editor/help.ts`
- Read: `frontend/src/editor/snippets.ts`
- Read: `platformio.ini`

**Step 1: Read the current docs and config**
- Extract hardware pins, brightness defaults, AP defaults, build commands, and DSL constructs.

**Step 2: Separate implemented vs planned behavior**
- Verify which live coding features are real on device and which are currently dev/mock only.

**Step 3: Draft section structure**
- Arrange sections from onboarding to advanced use.

### Task 2: Rewrite README

**Files:**
- Modify: `README.md`

**Step 1: Add hardware and wiring section**
- Document LED data pin, I2C pins, power constraints, and logical canvas.

**Step 2: Add quick start and build section**
- Include PlatformIO environment, frontend build/dev commands, and what each command does.

**Step 3: Add DSL reference**
- Document effect/sprite/layer syntax, variables, functions, and limits.

**Step 4: Add effect creation workflow**
- Explain new effect button, naming through `effect "..."`, validate/run/save flow.

**Step 5: Add LLM recommendations and copy-paste prompt**
- Provide strict guidance and example usage.

### Task 3: Verify documentation consistency

**Files:**
- Check: `README.md`

**Step 1: Read updated README for accuracy**
- Ensure no claims contradict firmware reality.

**Step 2: Run a lightweight verification command**
- Use a markdown readback or grep to confirm key sections were added.

**Step 3: Prepare summary for user**
- Highlight new sections and any explicit caveats.
