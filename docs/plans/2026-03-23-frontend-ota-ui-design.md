# Frontend OTA UI Design

**Date:** 2026-03-23

## Goal

Добавить в существующий встроенный frontend полноценную OTA-панель для firmware update flow, который уже реализован на устройстве:

- показать текущую OTA-сводку;
- дать переключение канала `stable`/`dev`;
- запускать `check` и `install` из браузера;
- не ломать текущий live-coding экран и его ритм работы.

## Scope

В этой фазе frontend получает:

- новую OTA-карточку в правой колонке;
- чтение текущего update state из firmware API;
- чтение и изменение update channel;
- кнопки `Проверить обновление` и `Установить`;
- отображение состояний `idle/checking/up-to-date/available/installing/completed/error`;
- отображение `availableVersion` и `updateError`.

Вне scope:

- отдельная страница или роутинг;
- прогресс-бар с точными процентами OTA download;
- автообновление без ручного подтверждения;
- история релизов и changelog.

## Chosen Approach

Выбран вариант с отдельной OTA-панелью в правой колонке текущего frontend.

Почему именно он:

- уже есть устойчивый single-screen UX без навигации;
- OTA логически относится к статусу устройства, а не к редактору кода;
- можно быстро встроить в существующий polling/status contract;
- минимальный риск сломать layout и embedded bundle.

Альтернативы, которые сознательно отклонены:

- отдельный экран OTA: избыточно для текущего объёма;
- размещение OTA в header: недостаточно места для channel/error/install state.

## UI Structure

Новая OTA-панель добавляется в правую колонку рядом с карточками runtime/status.

Состав панели:

- текущая версия прошивки;
- текущий канал обновлений;
- текущее состояние update subsystem;
- доступная версия, если найдена;
- последняя ошибка, если есть;
- selector канала `stable/dev`;
- кнопка `Проверить обновление`;
- кнопка `Установить`.

Поведение кнопок:

- `Проверить обновление` всегда доступна, кроме состояния активной операции;
- `Установить` активна только при наличии `availableVersion`;
- во время `checking` и `installing` обе кнопки блокируются;
- при `completed` UI показывает, что устройство уйдёт в reboot или уже перезапускается.

## Data Contract

Frontend использует уже существующие firmware endpoints:

- `GET /api/status`
- `GET /api/update/current`
- `GET /api/update/settings`
- `POST /api/update/settings`
- `POST /api/update/check`
- `POST /api/update/install`

Для рендера OTA-панели в frontend вводится отдельный тип update snapshot, чтобы не перегружать основной status type и не смешивать runtime/device fields с OTA fields.

## State Model

На клиенте добавляется лёгкий update store без фреймворка и без глобальной state library.

Store хранит:

- текущий `channel`;
- `state` update flow;
- `availableVersion`;
- `error`;
- флаг занятой операции.

Два потока данных остаются раздельными:

- текущий device status продолжает жить через `refreshStatus()`;
- OTA snapshot обновляется через отдельный `refreshUpdateState()`.

Это упрощает код: отказ OTA API не должен ломать основной экран статуса и live-coding.

## Interaction Flow

### Initial load

При старте страницы frontend делает:

1. `refreshStatus()`
2. `refreshUpdateState()`

Оба запроса независимы.

### Change channel

1. Пользователь выбирает `stable` или `dev`.
2. Frontend вызывает `POST /api/update/settings`.
3. После успешного ответа делает `refreshUpdateState()` и `refreshStatus()`.
4. В UI сразу отражается новый канал и очищается устаревшее сообщение, если backend его не вернул.

### Check update

1. Пользователь нажимает `Проверить обновление`.
2. Frontend блокирует OTA controls.
3. Вызывает `POST /api/update/check`.
4. После ответа делает `refreshUpdateState()`.
5. Если доступна новая версия, активирует кнопку установки.

### Install update

1. Пользователь нажимает `Установить`.
2. Frontend блокирует controls и показывает install state.
3. Вызывает `POST /api/update/install`.
4. После успешного ответа продолжает polling `GET /api/update/current`.
5. Если устройство уходит в reboot и временно недоступно, UI показывает ожидаемое сообщение вместо общей ошибки экрана.

## Error Handling

- Ошибки OTA должны отображаться локально внутри OTA-панели.
- Общая live-coding диагностика не переиспользуется для OTA действий.
- Если update API недоступен, панель должна показать деградированный state, но не ломать остальной интерфейс.
- Ошибки firmware отображаются близко к исходному тексту backend, без frontend-переформулировки.

## Dev And Mock Support

Mock backend расширяется OTA endpoint’ами с тем же shape, что и firmware:

- разные сценарии должны демонстрировать `up-to-date`, `available`, `error`, `installing` хотя бы на базовом уровне;
- reset dev scenario должен возвращать OTA mock state в исходное значение.

Это нужно для нормальной локальной разработки UI без устройства.

## Testing Strategy

Первый проход идёт по TDD:

1. добавить failing test или compile-time contract check для новых frontend типов и mock API;
2. прогнать его и увидеть red;
3. реализовать минимальный OTA client/UI;
4. собрать frontend `npm run build`;
5. при необходимости проверить firmware build только если меняется embed contract.

Минимальный обязательный verification set для этой фазы:

- frontend build проходит;
- mock dev server отвечает на OTA endpoint’ы;
- OTA controls рендерятся и выполняют базовые действия.