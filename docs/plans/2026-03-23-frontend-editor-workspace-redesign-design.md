# Frontend Editor Workspace Redesign Design

**Date:** 2026-03-23

## Goal

Пересобрать встроенный frontend вокруг редактора, чтобы основные действия и контекст лампы были доступны без постоянного просмотра правой колонки.

Новая компоновка должна дать:

- фиксированный нижний status bar с состоянием лампы;
- две круглые кнопки в header для сети и прошивки;
- editor-centered layout без тяжёлой правой sidebar;
- быстрый доступ к presets, playlist и help через dropdown-зоны рядом с редактором;
- reuse уже работающих modal-flow для сети и OTA.

## Scope

В этой фазе frontend получает:

- новый header с двумя round icon buttons `WiFi` и `FW`;
- новую editor workspace-зону с компактным dropdown dock;
- перенос hints, presets, playlist и help ближе к редактору;
- OTA panel в отдельную firmware modal;
- fixed bottom status bar;
- адаптацию current status rendering под новую разметку.

Вне scope:

- drag-and-drop playlist editor;
- поиск Wi-Fi сетей;
- полноценный preset browser с фильтрами;
- анимация перестройки layout между desktop и mobile beyond basic responsive behavior;
- новый backend API.

## Chosen Approach

Выбран editor-centered single-column workspace с dock-блоком прямо над редактором и fixed status bar снизу.

Почему этот вариант:

- редактор становится главным объектом интерфейса, что совпадает с live-coding сценарием;
- presets, playlist и help остаются рядом с кодом, а не в удалённой sidebar;
- сетевые и firmware actions становятся глобальными device actions в header;
- существующие modal flows уже готовы и хорошо подходят для редко используемых device settings.

Отклонённые варианты:

- оставить heavy sidebar и только добавить status bar: не решает проблему близости к editor;
- tabs вместо dropdown dock: tabs лучше скрывают один блок за счёт других, а user явно выбрал вариант с dropdown-подходом;
- вынести OTA в постоянную карточку: перегружает основной workspace вторичным device-admin контентом.

## UI Structure

### Header

Header остаётся компактным и содержит:

- бренд/заголовок;
- dev scenario controls в dev mode;
- две круглые кнопки действий:
  - `WiFi` открывает network modal;
  - `FW` открывает firmware modal.

Round buttons визуально читаются как device controls, а не как часть editor toolbar.

### Editor Workspace

Основная panel включает:

- action row `Новый эффект`, `Проверить`, `Запустить`, `Сохранить`;
- editor toolbar с hint/status;
- compact dropdown dock;
- code editor.

Dropdown dock реализуется как набор collapsible sections рядом с editor:

- `Идеи и пресеты`;
- `Очередь`;
- `Шпаргалка`;
- `Подсказки`.

По умолчанию секции занимают мало места и разворачиваются по требованию.

### Firmware Modal

Текущая OTA panel переносится в modal:

- summary/status note;
- текущая версия;
- канал;
- available version;
- last error;
- смена канала;
- check/install actions.

Таким образом OTA остаётся доступной, но не конкурирует с editor за постоянное место.

### Bottom Status Bar

Внизу экрана появляется фиксированная status strip c компактными runtime metrics:

- текущая версия/канал;
- preset;
- autoplay;
- playlist;
- сеть;
- время;
- sensor;
- температура;
- влажность.

Она всегда видна и заменяет старую runtime/status/sidebar зависимость.

## Interaction Model

### Presets / Help / Playlist

- user открывает нужный dropdown рядом с editor;
- chooses snippet or reads help без ухода в другую колонку;
- editor status and diagnostics обновляются как раньше.

### Device Controls

- `WiFi` в header открывает текущую network modal без изменений API behavior;
- `FW` в header открывает новую firmware modal, внутри которой работает уже существующая OTA logic.

### Runtime Awareness

- `refreshStatus()` обновляет нижний status bar;
- diagnostics summary остаётся в dropdown dock и не теряется;
- header больше не несёт постоянную нагрузку device telemetry.

## Data And State Impact

Новый backend contract не нужен.

Frontend state changes:

- добавить `firmwareModalOpen`;
- добавить open/close handlers для firmware modal;
- сохранить существующие OTA/network state variables;
- переназначить `renderStatus()` на новые DOM ids status bar.

## Responsive Behavior

- на desktop dropdown dock идёт в несколько колонок рядом с editor;
- на tablet/mobile dropdowns складываются в одну колонку;
- fixed status bar получает horizontal scroll/wrap-safe layout;
- main content получает нижний padding, чтобы status bar не перекрывал editor.

## Testing Strategy

Фаза идёт через TDD в два слоя:

1. добавить failing layout test для pure markup renderer;
2. реализовать новый shell markup и modal hooks;
3. прогнать существующий mock API test;
4. прогнать `tsc --noEmit` и production build;
5. прогнать firmware build из-за regeneration embedded assets.

Минимальный verification set:

- `cd /home/builder/mylamp/frontend && npm test`
- `cd /home/builder/mylamp/frontend && npm exec tsc --noEmit`
- `cd /home/builder/mylamp/frontend && npm run build`
- `cd /home/builder/mylamp && ~/.platformio/penv/bin/platformio run --environment esp32-c3-supermini-dev`