# Timezone Settings Modal Design

**Date:** 2026-03-23

## Goal

Добавить настройку часового пояса как отдельный device-settings flow, не перегружая header третьей кнопкой и не смешивая timezone с OTA или Wi-Fi form.

Фича должна дать:

- runtime-configurable timezone вместо compile-time `UTC0`;
- отдельную modal для времени и часового пояса;
- открытие по клику на clock block в нижнем status bar;
- сохранение timezone в persistent settings;
- пересинхронизацию часов после изменения.

## Scope

В этой фазе делаем:

- `GET /api/settings/time`;
- `POST /api/settings/time`;
- хранение timezone в `AppSettings.clock` и persistence backend;
- использование runtime timezone в `TimeRuntimeService`;
- frontend modal `Время и часовой пояс`;
- clock item в status bar как интерактивную точку входа.

Вне scope:

- ручная установка времени;
- изменение NTP серверов;
- поддержка произвольных IANA timezone names;
- timezone wizard с геолокацией.

## Chosen Approach

Выбран отдельный time modal, открываемый по клику на clock block в bottom status bar.

Почему это лучший вариант:

- header остаётся с двумя device buttons, как уже утверждено;
- timezone логически привязан к отображаемому времени;
- network modal не превращается в свалку несвязанных настроек;
- backend получает отдельный settings API по уже существующему паттерну `network` и `update`.

## Backend Design

### Settings model

В `settings::AppSettings` поле `clock` расширяется строкой timezone.

- default: `config::kTimeZone`;
- persistence key: `clock.timezone`;
- normalization: только whitelist значений из поддерживаемого списка.

### Time source flow

`TimeRuntimeService` больше не берёт timezone из compile-time константы напрямую. Вместо этого runtime service получает timezone из `ClockSettings`.

При смене timezone:

- settings сохраняются в Preferences;
- runtime state пересчитывается;
- при разрешённом NTP sync выполняется новый `configTzTime` с выбранной зоной.

### API contract

`GET /api/settings/time` возвращает:

- `timezone`

`POST /api/settings/time` принимает:

- `timezone`

На invalid timezone backend возвращает `400`.

## Frontend Design

### Entry point

Нижний status bar clock item становится button-like element.

Клик открывает modal `Время и часовой пояс`.

### Modal content

Modal содержит:

- summary/status text;
- select с коротким whitelist timezone preset values;
- save button;
- close button.

### Supported timezone list

В первой версии используем фиксированный список TZ strings, которые гарантированно понимает ESP32/newlib:

- `UTC0`
- `EET-2EEST,M3.5.0/3,M10.5.0/4`
- `MSK-3`
- `CET-1CEST,M3.5.0,M10.5.0/3`
- `EST5EDT,M3.2.0/2,M11.1.0/2`
- `PST8PDT,M3.2.0/2,M11.1.0/2`

UI показывает human-readable labels, backend хранит actual TZ string.

## Testing Strategy

Фича идёт через TDD:

1. failing firmware test для persistence timezone;
2. failing firmware test для time settings JSON/apply helpers;
3. failing firmware test для runtime service using configured timezone;
4. failing frontend mock API test для `/api/settings/time`;
5. failing layout test для clock action hook;
6. implementation;
7. full verification.

Минимальный verification set:

- `cd /home/builder/mylamp && ~/.platformio/penv/bin/platformio test -e native-test -f test_settings_persistence -f test_time_runtime -f test_time_settings_api`
- `cd /home/builder/mylamp/frontend && npm test`
- `cd /home/builder/mylamp/frontend && npm exec -- tsc --noEmit`
- `cd /home/builder/mylamp/frontend && npm run build`
- `cd /home/builder/mylamp && ~/.platformio/penv/bin/platformio run --environment esp32-c3-supermini-dev`