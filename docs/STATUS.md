# Project Status

Этот документ фиксирует реальное состояние `mylamp` на текущий момент, без маркетинга и без обещаний «почти готово» там, где код ещё не доведён до конца.

## Уже работает

### Firmware foundation

- базовая сборка под `esp32-c3-supermini-dev`;
- аппаратные константы и runtime-конфиг;
- логическая поверхность `32x16` поверх двух панелей `16x16`;
- сетевой и time runtime;
- polling AHT30;
- web server и набор API для статуса, preset-ов и playlist-ов.

### Live coding runtime на устройстве

- DSL парсер, компилятор и executor работают на ESP32;
- `POST /api/live/validate` — реальная валидация с диагностикой (номер строки, текст ошибки);
- `POST /api/live/run` — компиляция и запуск эффекта на лампе;
- поддерживаемые операторы: `+`, `-`, `*`, `/`, `%`;
- поддерживаемые функции: `sin`, `cos`, `abs`, `min`, `max`, `clamp`, `temp()`, `humidity()`;
- цвет через `rgb(...)` и `hsv(...)`;
- layer-поля `rotation` и `blend` работают в runtime;
- переменные: `t`, `dt`, `x`, `y`, `nx`, `ny`.

### Frontend

- локальный frontend на Vite + TypeScript;
- editor-first UI для написания эффектов;
- быстрые кнопки Wi-Fi, timezone и OTA в шапке;
- правая tabbed-панель `Идеи` / `Лампа` / `Справка` рядом с редактором;
- кнопки `Проверить`, `Запустить`, `Сохранить`, `Новый`;
- preset manager UI: список, открытие в редактор, активация на лампе, удаление, подсветка активного preset-а;
- playlist manager UI: список, запуск, остановка, удаление, inline editor для `repeat` и entries;
- OTA UI: retry/recovery flow после install, mock-сценарий ошибки обновления;
- mock backend для локальной разработки без железа;
- сценарии `happy-path`, `autoplay`, `dsl-error`, `offline-ish`, `sensor-missing`.

### Render pipeline

- effect/render pipeline разделён на effect pass, overlay pass и commit stage;
- clock overlay рисуется поверх эффекта в firmware runtime;
- есть unit tests для overlay rendering.

### Workflow

- можно локально писать DSL и гонять UI без устройства;
- можно сохранять preset-ы через существующий API;
- можно управлять playlist-ами из browser UI;
- release contract и базовый CI/release flow документированы;
- можно использовать `docs/LLM_EFFECT_PROMPT.md` как жёсткий шаблон для генерации эффектов.

## Частично готово

### OTA direction

- release contract, frontend UX и CI-пайплайн уже есть;
- dev/stable channels и GitHub Release публикация описаны и частично автоматизированы;
- но полный production-grade update path с device-side safety guarantees и rollback strategy ещё не закрыт end-to-end.

### Product polish

- editor workflow рабочий, но без локального diff/sync UX между текущим текстом и сохранённым preset;
- playlist/preset UI закрывает основной сценарий, но без расширенных шаблонов, истории и revert flow;
- roadmap blocks 1-5 исполнены, следующий backlog уже про polish и production hardening.

## Ещё не реализовано

- production-grade OTA workflow end-to-end с проверяемой device-side safety моделью;
- пользовательские функции и более широкий DSL поверх текущих `rotation`/`blend`;
- углублённый content-sync UX: revert, diff, richer templates, history.

## Что имеет смысл делать дальше

Приоритетные следующие шаги:

1. Добить production-grade OTA path: checksum/boot safety/rollback strategy и реальная device validation.
2. Определить следующий DSL backlog после `rotation` и `blend`: нужны ли user functions, новые color ops или layer semantics.
3. Добавить product polish для content workflow: sync/revert/history/templates.

## Как читать этот статус правильно

Если тебе нужно быстро понять проект:

- README отвечает на вопрос «что это и как запустить»;
- [docs/DSL.md](DSL.md) отвечает на вопрос «какой тут язык эффектов»;
- этот файл отвечает на вопрос «что реально работает прямо сейчас»;
- [docs/ROADMAP.md](ROADMAP.md) отвечает на вопрос «что осталось и в каком порядке это делать».