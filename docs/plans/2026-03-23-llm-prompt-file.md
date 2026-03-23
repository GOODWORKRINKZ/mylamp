# LLM Prompt File Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Вынести prompt для генерации DSL-эффектов в отдельный markdown-файл и связать его с README без дублирования длинного текста.

**Architecture:** Основной copy-paste prompt живёт в `docs/LLM_EFFECT_PROMPT.md`. README остаётся обзорным документом и ссылается на новый файл. Это снижает перегрузку README и делает prompt удобным для прямого копирования.

**Tech Stack:** Markdown, существующий README, текущий DSL contract.

---

### Task 1: Create dedicated prompt file

**Files:**
- Create: `docs/LLM_EFFECT_PROMPT.md`

**Step 1: Add purpose and usage notes**
- Explain when to use the prompt and what result it should produce.

**Step 2: Add the copy-paste prompt**
- Use only currently supported DSL constructs.

**Step 3: Add request/response examples**
- Show a realistic user request and an expected DSL-only output.

### Task 2: Update README

**Files:**
- Modify: `README.md`

**Step 1: Keep LLM guidance section**
- Preserve recommendations and constraints for model usage.

**Step 2: Replace long inline prompt block**
- Point readers to `docs/LLM_EFFECT_PROMPT.md`.

**Step 3: Keep a short usage example**
- Leave enough context in README so readers know how to apply the prompt file.

### Task 3: Verify docs consistency

**Files:**
- Check: `README.md`
- Check: `docs/LLM_EFFECT_PROMPT.md`

**Step 1: Re-read both files**
- Confirm README and prompt file do not contradict each other.

**Step 2: Verify new links/sections exist**
- Use a quick search or readback.

**Step 3: Summarize resulting workflow**
- README for overview, dedicated prompt file for copy/paste.
