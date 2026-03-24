# Roadmap: Конкретный план работ

Здесь описано, что осталось сделать, в каком порядке и какие файлы при этом трогаем. Каждая задача — готовый пункт для начала исполнения, а не абстрактное направление.

## Текущая картина

Backend и firmware уже умеют заметно больше, чем показывает frontend:

| Возможность | Backend | Mock API | Frontend UI |
| --- | --- | --- | --- |
| Сохранить preset | ✅ PUT | ✅ | ✅ `savePreset()` |
| Список preset-ов | ✅ GET /api/presets | ✅ | ❌ нет вызова |
| Загрузить preset | ✅ GET /api/presets/:id | ✅ | ❌ нет вызова |
| Удалить preset | ✅ DELETE /api/presets/:id | ✅ | ❌ нет UI |
| Активировать preset | ✅ POST /api/presets/:id/activate | ✅ | ❌ нет кнопки |
| Создать playlist | ✅ PUT /api/playlists/:id | ✅ | ❌ нет UI |
| Стартовать playlist | ✅ POST /api/playlists/:id/start | ✅ | ❌ нет кнопки |
| Остановить playlist | ✅ POST /api/playlists/stop | ✅ | ❌ нет кнопки |
| Удалить playlist | ✅ DELETE /api/playlists/:id | ✅ | ❌ нет UI |
| OTA check / install | ✅ | ✅ | ✅ модалка |
| Release pipeline | Частично | — | — |

Порядок работ: UI preset-ов → UI playlist-ов → OTA hardening → render pipeline → расширение DSL.

---

## Блок 1. Preset Manager UI

Цель: пользователь может из web UI посмотреть все сохранённые эффекты, открыть любой в редактор, активировать на лампе и удалить ненужные.

### Задача 1.1. Загрузка списка preset-ов

**Что сделать:**
Добавить в `main.ts` функцию `loadPresetList()`, которая вызывает `GET /api/presets` и получает `{ items: [{ id, name, updatedAt }] }`. Результат хранить в переменной `presetListItems`. Вызывать при загрузке страницы и после каждого сохранения/удаления.

**Файлы:**
- `frontend/src/main.ts` — добавить `loadPresetList()`, переменную состояния, вызов при init.

**API:** `GET /api/presets` → `{ items: [{ id: string, name: string, updatedAt: string }] }`

**Результат:** после загрузки страницы frontend знает, какие preset-ы есть на лампе.

---

### Задача 1.2. Отрисовать список preset-ов в правой панели

**Что сделать:**
В sidebar-панели «Идеи» (id=`tab-presets`) под списком starter snippet-ов добавить секцию «Сохранённые». Для каждого preset-а рисовать строку с именем, датой обновления и кнопками действий.

**Разметка одного элемента:**
```
[имя preset-а] [updatedAt]
[Открыть] [▶ На лампу] [🗑]
```

**Файлы:**
- `frontend/src/main.ts` — функция `renderPresetList()`, которая обновляет DOM внутри `tab-presets`.
- `frontend/src/ui/shellTemplate.ts` — добавить `<div id="saved-presets-list"></div>` после `<ul class="item-list">`.
- `frontend/src/styles/app.css` — стили для `.preset-item`, `.preset-item__actions`.

**Результат:** пользователь видит свои сохранённые эффекты рядом со starter-идеями.

---

### Задача 1.3. Открыть preset в редактор

**Что сделать:**
По клику «Открыть» на preset-е — вызвать `GET /api/presets/:id`, получить полный `PresetPayload` c полем `source`, подставить `source` в textarea `#editor-code`. Запомнить текущий `presetId` в переменную `activeEditingPresetId`.

**Файлы:**
- `frontend/src/main.ts` — добавить `loadPresetIntoEditor(presetId)`, использовать `GET /api/presets/:id`.

**API:** `GET /api/presets/:id` → `PresetPayload { id, name, source, createdAt, updatedAt, tags, options }`

**Результат:** клик на preset загружает его код в редактор без ручного копирования.

---

### Задача 1.4. Активировать preset на лампе

**Что сделать:**
По клику «▶ На лампу» — вызвать `POST /api/presets/:id/activate`. После успешного ответа вызвать `refreshStatus()`, чтобы statusbar обновился. При ошибке (400 — не компилируется, 404 — не найден) показать сообщение в diagnostics.

**Файлы:**
- `frontend/src/main.ts` — добавить `activatePreset(presetId)`.

**API:** `POST /api/presets/:id/activate` → `{ ok, activePresetId, temporary, autoplayActive }`

**Результат:** эффект запускается на лампе одной кнопкой без копирования кода в редактор.

---

### Задача 1.5. Удалить preset

**Что сделать:**
По клику «🗑» — спросить подтверждение ("Удалить preset «имя»?"), при OK вызвать `DELETE /api/presets/:id`. После успеха перезагрузить список preset-ов (`loadPresetList()`). При ошибке — показать сообщение.

**Файлы:**
- `frontend/src/main.ts` — добавить `deletePreset(presetId, presetName)`.

**API:** `DELETE /api/presets/:id` → `{ ok: true }` или 404.

**Результат:** можно убрать ненужные эффекты из памяти лампы.

---

### Задача 1.6. Умное сохранение: update vs create

**Что сделать:**
Текущий `savePreset()` всегда делает PUT с новым ID, сгенерированным из имени. Нужно:
- Если `activeEditingPresetId` задан и имя не менялось — обновлять существующий preset.
- Если имя изменилось или preset новый — создавать с новым ID.
- После сохранения перезагрузить список preset-ов.

**Файлы:**
- `frontend/src/main.ts` — доработать `savePreset()`.

**Результат:** повторное сохранение не плодит дубликаты, а обновляет тот же preset.

---

### Задача 1.7. Подсветить активный preset

**Что сделать:**
В списке preset-ов выделять тот, который сейчас активен на лампе (`activePresetId` из `StatusPayload`). Для этого при рендере списка сравнивать `item.id` с `status.activePresetId` и добавлять CSS-класс `preset-item--active`.

**Файлы:**
- `frontend/src/main.ts` — передавать `activePresetId` в `renderPresetList()`.
- `frontend/src/styles/app.css` — стиль `.preset-item--active`.

**Результат:** сразу видно, какой эффект сейчас горит на лампе.

---

## Блок 2. Playlist Editor UI

Цель: пользователь может собрать очередь из preset-ов, задать длительности и запустить автосмену на лампе.

### Задача 2.1. Загрузка списка playlist-ов

**Что сделать:**
Добавить функцию `loadPlaylists()`. Сейчас mock endpoint `GET /api/presets` уже возвращает `playlists` в scenario state, но отдельного list endpoint для playlist-ов нет — нужно проверить, есть ли `GET /api/playlists` на firmware. Если нет — добавить.

**Проверить:**
- `src/web/LampWebServer.cpp` — зарегистрирован ли `GET /api/playlists`?
- Если нет — добавить `handleListPlaylists()` по аналогии с `handleListPresetsRequest()`.

**Файлы (frontend):** `frontend/src/main.ts` — `loadPlaylists()`.
**Файлы (backend, если нужно):** `src/web/PlaylistApi.cpp`, `src/web/LampWebServer.cpp`.
**Файлы (mock, если нужно):** `frontend/mockApi.mjs`.

**Результат:** frontend знает, какие playlist-ы есть.

---

### Задача 2.2. UI playlist editor в sidebar-панели «Лампа»

**Что сделать:**
Переделать таб «Лампа» (id=`tab-queue`) из display-only в полноценную панель:

**Секция «Текущее состояние»:**
- Какой preset сейчас активен (уже есть).
- Какой playlist сейчас крутится (уже есть).
- Кнопка «Остановить очередь» (POST /api/playlists/stop).

**Секция «Плейлисты»:**
- Список playlist-ов с кнопками «▶ Запустить» и «🗑 Удалить».
- Кнопка «Новый плейлист».

**Файлы:**
- `frontend/src/ui/shellTemplate.ts` — переделать разметку `tab-queue`.
- `frontend/src/main.ts` — `renderPlaylistPanel()`, `stopPlaylist()`, `startPlaylist(id)`, `deletePlaylist(id)`.
- `frontend/src/styles/app.css` — стили для playlist UI.

**API:**
- `POST /api/playlists/:id/start` → `{ ok, activePlaylistId, active, autoplayActive, activeEntryIndex }`
- `POST /api/playlists/stop` → `{ ok: true }`
- `DELETE /api/playlists/:id` → `{ ok: true }`

**Результат:** из таба «Лампа» можно запустить, остановить и удалить playlist.

---

### Задача 2.3. Форма создания и редактирования playlist-а

**Что сделать:**
При клике «Новый плейлист» или на существующий playlist — показать inline-форму или модалку:

- Поле `name`.
- Чекбокс `repeat`.
- Список entries. Каждый entry:
  - выпадающий список `presetId` (из загруженного preset list).
  - поле `durationSec` (числовое).
  - чекбокс `enabled`.
  - кнопка «Убрать entry».
- Кнопка «Добавить entry».
- Кнопки «Сохранить» / «Отмена».

**Файлы:**
- `frontend/src/main.ts` — `renderPlaylistEditor(playlist?)`, `savePlaylist(id, payload)`.

**API:** `PUT /api/playlists/:id` с телом `{ name, repeat, entries: [{ presetId, durationSec, enabled }] }`.

**Результат:** можно собрать playlist из имеющихся preset-ов и сохранить.

---

### Задача 2.4. Runtime sync: playlist state в statusbar

**Что сделать:**
Текущий `refreshStatus()` уже читает `activePlaylistId` и `autoplayEnabled` из `StatusPayload`. Нужно после start/stop playlist-а вызывать `refreshStatus()` и обновлять:
- statusbar-playlist: имя активного playlist-а (а не только ID).
- statusbar-autoplay: «Вкл» / «Выкл».
- runtime-playlist в sidebar.

**Файлы:**
- `frontend/src/main.ts` — доработать рендеринг playlist state.

**Результат:** видно, какой playlist сейчас крутится и на каком entry.

---

## Блок 3. OTA Hardening

Цель: OTA-обновление воспроизводимо из чистого репозитория и устойчиво к типичным сбоям.

### Задача 3.1. Документировать release contract

**Что сделать:**
Написать `docs/RELEASE.md`:
- какой артефакт публикуется (бинарник, checksum, metadata);
- как каналы `dev` / `stable` маппятся на GitHub Releases tags;
- как firmware определяет, что есть обновление;
- что считается валидным кандидатом на update.

**Файлы:** `docs/RELEASE.md` (новый).

**Результат:** любой участник проекта понимает, как выпустить обновление.

---

### Задача 3.2. CI: автоматическая сборка release-артефактов

**Что сделать:**
Добавить GitHub Actions workflow, который:
- собирает `esp32-c3-supermini-release`;
- генерирует checksum;
- публикует артефакт в GitHub Release при тегировании.

**Файлы:** `.github/workflows/release.yml` (новый или расширение существующего).

**Результат:** релиз делается через `git tag` + push, без ручной сборки.

---

### Задача 3.3. Укрепить post-install UX

**Что сделать:**
Сейчас после install frontend пишет «Устройство перезагружается» и пытается переопросить `/api/update/current`. Нужно:
- Добавить цикл повторных попыток с задержкой (устройство перезагружается 5-15 сек).
- Показывать осмысленный прогресс: «Перезагрузка... попытка 1/5».
- При восстановлении связи — показать новую версию.

**Файлы:** `frontend/src/main.ts` — доработать `installUpdate()` и `refreshUpdateState()`.

**Результат:** OTA-установка не выглядит как "всё сломалось", а показывает понятный процесс.

---

### Задача 3.4. Mock-сценарий для OTA-ошибок

**Что сделать:**
Добавить dev-сценарий `ota-error`:
- check возвращает ошибку «Не удалось получить releases».
- Или install возвращает ошибку «checksum mismatch».

**Файлы:**
- `frontend/src/dev/mockScenarios.ts` — новый сценарий.
- `frontend/mockApi.mjs` — обработка ошибочных OTA-ответов.

**Результат:** OTA-ошибки можно тестировать локально без реального firmware.

---

## Блок 4. Render Pipeline: Overlay и Effect Engine

Цель: часы и служебные индикаторы рисуются поверх эффекта без смешивания логики в effect runtime.

### Задача 4.1. Выделить render loop из main.cpp

**Что сделать:**
Сейчас в `loop()` в `main.cpp` рендер живёт inline. Нужно выделить:
- `effectRender()` — рисует active effect в framebuffer.
- `overlayRender()` — рисует системные overlay поверх (часы, индикаторы).
- `commitFrame()` — отправляет framebuffer на LED.

**Файлы:**
- `src/main.cpp` — рефакторинг `loop()`.
- (Опционально) `src/effects/EffectEngine.h/.cpp` — если выносим в отдельный модуль.

**Результат:** render pipeline стал явным: effect → overlay → commit.

---

### Задача 4.2. Минимальный clock overlay

**Что сделать:**
Если clock enabled в настройках и NTP-время доступно — рисовать текущее время поверх эффекта в заданной позиции. Начать с простейшей реализации: несколько пикселей как «точка-индикатор», не bitmap-шрифт.

**Файлы:**
- `src/effects/ClockOverlay.h/.cpp` (новые).
- `src/main.cpp` — подключить overlay в render pipeline.

**Результат:** на лампе видно время поверх эффекта без правки DSL.

---

## Блок 5. Расширение DSL

Цель: добавить в язык возможности, которые реально нужны для новых визуальных сценариев.

### Задача 5.1. Rotation для sprite

**Что сделать:**
Добавить в `layer` опциональное свойство `rotation = <expression>` (в градусах или радианах). Парсер, компилятор и executor должны поддерживать новое поле. При отсутствии — считать `0`.

**Файлы:**
- `include/live/dsl/Ast.h` — поле `rotationExpression` + `rotationLine`.
- `src/live/dsl/Parser.cpp` — парсинг `rotation = ...`.
- `include/live/runtime/CompiledProgram.h` — поле `rotationExpression`.
- `src/live/runtime/Compiler.cpp` — компиляция.
- `src/live/runtime/Executor.cpp` — применение rotation при рендере.
- `test/test_dsl_parser/` — тест парсинга.
- `test/test_dsl_executor/` — тест рантайма.
- `docs/DSL.md` — обновить спецификацию.
- `docs/LLM_EFFECT_PROMPT.md` — обновить промпт.

**Результат:** `rotation = t * 45` вращает спрайт.

---

### Задача 5.2. Blend modes для layer

**Что сделать:**
Добавить в `layer` опциональное свойство `blend = <mode>`. Варианты: `normal` (default), `add`, `multiply`. Реализовать в executor при наложении layer на framebuffer.

**Файлы:** аналогично Задаче 5.1 — parser, compiler, executor, тесты, документация.

**Результат:** `blend = add` делает аддитивное наложение — основа для glow-эффектов.

---

## Порядок выполнения

```
Блок 1: Preset Manager UI
  1.1 → 1.2 → 1.3 → 1.4 → 1.5 → 1.6 → 1.7
  ↓
Блок 2: Playlist Editor UI
  2.1 → 2.2 → 2.3 → 2.4
  ↓
Блок 3: OTA Hardening
  3.1 → 3.2 → 3.3 → 3.4
  ↓
Блок 4: Render Pipeline
  4.1 → 4.2
  ↓
Блок 5: DSL
  5.1 → 5.2
```

Блоки 1 и 2 — frontend-only, не трогают firmware. Можно разрабатывать и тестировать на mock-бэкенде без лампы.

Блок 3 — frontend + CI + документация.

Блоки 4 и 5 — firmware C++ с native-тестами.

---

## Контрольные точки

| После | Что должно работать |
| --- | --- |
| Блок 1 | Из browser: список preset-ов, открытие в редактор, активация на лампе, удаление |
| Блок 2 | Из browser: создание и запуск playlist-а, видимость текущего состояния |
| Блок 3 | `git tag` → CI → GitHub Release → лампа обновляется через OTA |
| Блок 4 | Часы/индикаторы горят поверх эффекта |
| Блок 5 | `rotation = t * 90` вращает спрайт, `blend = add` делает glow |