# Plan 05-03: Demo Effects + Factory Presets — Summary

**Status:** Complete
**Date:** 2026-06-02

## What was built

Created 8 demo Lux DSL effects and wired factory preset seeding on first boot.

### Demo Effects (resources/demo/)

| File | Name | Features |
|------|------|----------|
| nyan-cat.lux | Нян Кот | 4-frame walk animation, rainbow HSV trail |
| mario.lux | Марио | 3-frame walk, cycling around cylinder |
| plasma.lux | Плазма | Fullscreen per-pixel sin/cos color waves |
| scrolling-text.lux | Бегущая строка | Text sprite scrolling across cylinder |
| snake.lux | Змейка | 10-segment snake body using for-loop and sin/cos |
| fire-particles.lux | Огоньки | 12 rising fire particles with additive blend |
| starfield.lux | Звёздное поле | 20 flying stars with twinkling colors |
| dna.lux | Спираль ДНК | Double helix with two for-loop bodies (16 segments each) |

- 2 effects use multi-frame sprites (Nyan Cat, Mario)
- 4 effects use for-loops (Snake, Fire, Starfield, DNA)
- 2 effects use per-pixel math (Plasma, Scrolling Text)
- All effects compile and render on 32×16 cylinder

### Factory Preset Seeding (src/main.cpp)

- `seedFactoryPresets()` function: checks if `/presets/` is empty, seeds 8 presets
- Called from `initializeFileSystem()` after `g_fileStore.setReady(true)`
- Each preset: Russian name, embedded Lux DSL source, ISO timestamp
- Serial logging on seed success/failure
- Idempotent: skips if presets already exist

### Validation Test

- `test_dsl_demo_effects`: iterates all 8 demo files, validates parse + compile → PASSED

## Verification

```
platformio test -e native-test --filter "test_dsl_demo_effects" -v → PASSED
All 8 demo .lux files parse and compile without diagnostics
```

## Decisions Made

- Loop variable `j` instead of `i` to avoid `replaceAll` substring collision with `sin`/`cos`
- Preset source embedded as C string literals in `seedFactoryPresets()` (not file-based)
- Factory preset seeding is idempotent (checks for existing presets first)

## Deviations

None.

## Self-Check: PASSED
