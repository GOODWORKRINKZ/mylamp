# Frontend Network Modal Design

**Date:** 2026-03-23

## Goal

Добавить в существующий встроенный frontend отдельную модалку настройки сети, открываемую по кнопке из карточки статуса лампы.

Модалка должна позволять:

- читать текущие network settings с устройства;
- переключать режим `AP` / `Client`;
- задавать `accessPointName`, `clientSsid`, `clientPassword`;
- сохранять настройки без перехода на отдельную страницу.

## Scope

В этой фазе frontend получает:

- кнопку `Настроить сеть` в карточке `Как себя чувствует лампа`;
- modal overlay с формой network settings;
- чтение значений через `GET /api/settings/network`;
- сохранение через `POST /api/settings/network`;
- локальную индикацию ошибок и статуса сохранения.

Вне scope:

- поиск Wi-Fi сетей с устройства;
- отдельный wizard первичной настройки;
- парольные политики и strength meter;
- автоматическая проверка доступности сети до сохранения.

## Chosen Approach

Выбран вариант с кнопкой в карточке состояния сети и отдельной модалкой.

Почему именно он:

- сеть относится к состоянию устройства, а не к редактору DSL;
- форма не занимает постоянное место в sidebar;
- mobile UX остаётся компактным;
- существующий frontend уже живёт как single-screen control surface, и модалка хорошо вписывается в этот паттерн.

Отклонённые варианты:

- кнопка в header: слишком перегружает верхнюю панель;
- постоянная карточка с полями: создаёт визуальный шум и расширяет sidebar без постоянной пользы.

## UI Structure

### Entry point

В карточке `Как себя чувствует лампа` появляется кнопка `Настроить сеть`.

### Modal content

Модалка содержит:

- selector режима `AP` / `Client`;
- поле `Имя точки доступа`;
- поле `SSID`;
- поле `Пароль`;
- краткое пояснение по выбранному режиму;
- статусную строку для ошибок/успешного сохранения;
- кнопки `Сохранить` и `Закрыть`.

### Conditional behavior

- при `mode=ap` поля `SSID` и `Пароль` становятся disabled;
- при `mode=client` все поля доступны;
- `accessPointName` остаётся редактируемым в обоих режимах, чтобы сохранить fallback AP name.

## Data Contract

Используется уже существующий firmware API:

- `GET /api/settings/network`
- `POST /api/settings/network`

Payload из `GET`:

- `mode`
- `accessPointName`
- `clientSsid`

Payload в `POST`:

- `mode`
- `accessPointName`
- `clientSsid`
- `clientPassword`

Формат отправки остаётся `application/x-www-form-urlencoded`, чтобы полностью совпадать с текущим firmware handler.

## Client State Model

Добавляется локальный modal state:

- `isOpen`
- `isLoading`
- `isSaving`
- значения формы
- локальный статусный текст

Этот state не смешивается с OTA state и DSL diagnostics.

## Interaction Flow

### Open modal

1. Пользователь нажимает `Настроить сеть`.
2. Frontend открывает modal shell.
3. Сразу вызывает `GET /api/settings/network`.
4. Заполняет форму актуальными значениями.

### Save settings

1. Пользователь меняет режим и/или поля.
2. Нажимает `Сохранить`.
3. Frontend блокирует кнопки формы.
4. Отправляет `POST /api/settings/network`.
5. После успеха показывает подтверждение и обновляет `refreshStatus()`.

### Close modal

- по кнопке `Закрыть`;
- по overlay click;
- по `Escape`, если это не ломает текущую структуру handwritten DOM-кода.

## Error Handling

- ошибки сети показываются только внутри модалки;
- если `GET` не удался, модалка остаётся открытой и показывает текст ошибки;
- если `POST` не удался, форма не закрывается;
- общая панель DSL/OTA не переиспользуется для network feedback.

## Dev And Mock Support

Mock backend нужно расширить network settings endpoint’ами, чтобы modal flow работал локально без железа.

Минимум для mock:

- `GET /api/settings/network` возвращает seeded значения;
- `POST /api/settings/network` сохраняет изменения в scenario state;
- `__dev/reset` возвращает network settings к исходным значениям.

## Testing Strategy

Фаза идёт по TDD:

1. добавить failing integration test для network settings endpoints в mock API;
2. увидеть red;
3. реализовать минимальный mock contract;
4. добавить modal UI и client logic;
5. прогнать frontend test/build;
6. проверить firmware build из-за regeneration embedded assets.

Минимальный verification set:

- `npm test` в `frontend` проходит;
- `npm run build` в `frontend` проходит;
- `platformio run --environment esp32-c3-supermini-dev` проходит.