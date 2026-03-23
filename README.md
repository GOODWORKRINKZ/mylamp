# mylamp

Пиксельная лампа на ESP32-C3 Super Mini: две матрицы WS2812B 16x16 складываются в цилиндр, а поверх этого работают firmware, web UI и live-coding workflow для собственных эффектов.

## Что это

`mylamp` нужен для трёх вещей:

- собрать прошивку для лампы на ESP32-C3;
- настраивать устройство через встроенный web UI;
- придумывать и запускать эффекты через маленький DSL, не копаясь каждый раз в C++.

Проект специально разделён на две части:

- `firmware` на PlatformIO + Arduino/C++;
- `frontend` на Vite + TypeScript, который можно гонять локально даже без железа.

## Что уже есть

Сейчас в репозитории уже собраны базовые кирпичи:

- логический canvas `32x16` поверх двух панелей `16x16`;
- firmware-скелет под ESP32-C3 Super Mini;
- web API статуса, preset-ов и playlist-ов;
- web UI с editor-first workflow;
- local dev режим со стабовым backend без реальной лампы;
- базовый DSL-контракт для live coding;
- работа с временем, Wi-Fi состоянием и AHT30 внутри runtime.

Быстрый честный статус проекта лежит отдельно:

- [docs/STATUS.md](docs/STATUS.md)

## Быстрый старт

### Firmware build

```bash
pio run -e esp32-c3-supermini-dev
```

### Upload на устройство

```bash
pio run -e esp32-c3-supermini-dev -t upload
```

### Serial monitor

```bash
pio device monitor -b 115200
```

### Локальная разработка frontend без железа

```bash
cd frontend
npm install
npm run dev
```

В dev-режиме поднимается Vite и mock backend. Через него можно гонять UI, проверять editor workflow и сценарии без ESP32.

### Frontend production build

```bash
cd frontend
npm run build
```

Собранные ресурсы попадают в `resources/dist/` и потом встраиваются в firmware.

## Железо

Целевая плата:

- ESP32-C3 Super Mini

Периферия:

- 2x WS2812B 16x16
- AHT30 по I2C

Логическая поверхность:

| Параметр | Значение |
| --- | --- |
| Ширина | `32` |
| Высота | `16` |
| Всего пикселей | `512` |

Текущие firmware-константы:

| Назначение | Значение |
| --- | --- |
| LED data | `GPIO2` |
| I2C SDA | `GPIO8` |
| I2C SCL | `GPIO9` |
| Brightness default | `32` |
| Brightness cap | `96` |

## Подключение

Минимальная схема:

1. Обе матрицы WS2812B питаются от внешнего блока питания.
2. `DIN` первой матрицы подключается к `GPIO2`.
3. `DOUT` первой матрицы идёт в `DIN` второй, если панели соединены последовательно.
4. AHT30 подключается по I2C:
   - `SDA` -> `GPIO8`
   - `SCL` -> `GPIO9`
5. У ESP32, матриц и датчика должна быть общая `GND`.

Практические замечания:

- не стоит питать обе матрицы от USB самой ESP32-C3;
- если начинаются глюки, сначала проверяй питание и землю;
- логическое отображение панелей централизовано в `MatrixLayout`, поэтому физические перевороты лучше править в firmware, а не в DSL.

## Web UI и эффекты

Текущий workflow короткий:

1. Открываешь web UI.
2. Нажимаешь `Новый эффект` или загружаешь готовую идею.
3. Пишешь DSL в редакторе.
4. В первой строке задаёшь имя:

```txt
effect "my_effect"
```

5. `Проверить` отправляет код на `POST /api/live/validate`.
6. `Запустить` отправляет код на `POST /api/live/run`.
7. `Сохранить` сохраняет preset через `PUT /api/presets/:presetId`.

Полная спецификация языка вынесена сюда:

- [docs/DSL.md](docs/DSL.md)

Готовый copy-paste prompt для LLM лежит отдельно:

- [docs/LLM_EFFECT_PROMPT.md](docs/LLM_EFFECT_PROMPT.md)

## Local Dev Scenarios

Mock backend поддерживает несколько сценариев:

- `happy-path`
- `autoplay`
- `dsl-error`
- `offline-ish`
- `sensor-missing`

Сценарий можно выбрать через dev-панель в UI или через query string, например:

```txt
http://127.0.0.1:5173/?scenario=autoplay
```

## Основные API

Ключевые маршруты, вокруг которых уже построен UI:

- `GET /api/status`
- `POST /api/live/validate`
- `POST /api/live/run`
- `GET /api/presets`
- `PUT /api/presets/:id`
- `POST /api/presets/:id/activate`
- `POST /api/playlists/:id/start`
- `POST /api/playlists/stop`

## Структура репозитория

- `platformio.ini` — PlatformIO environments
- `include/AppConfig.h` — аппаратные и runtime-константы
- `src/main.cpp` — сборка runtime и сервисов
- `src/web/` — HTTP API и web-server glue
- `src/live/` — live coding runtime, DSL и storage services
- `frontend/` — web UI и local dev tooling
- `resources/dist/` — frontend build для embedding
- `docs/` — архитектура, DSL, статус и планы

## Куда читать дальше

- [docs/STATUS.md](docs/STATUS.md) — что уже работает, что ещё не доделано
- [docs/DSL.md](docs/DSL.md) — синтаксис языка эффектов и примеры
- [docs/LLM_EFFECT_PROMPT.md](docs/LLM_EFFECT_PROMPT.md) — готовый prompt для генерации эффектов через LLM
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) — верхнеуровневая архитектура и направление runtime

## OTA и release direction

Проект целится в release-схему по образцу microbbox:

- GitHub Releases как источник OTA-артефактов;
- отдельные `dev` и `stable` каналы;
- версия и канал, вшитые в build flags.

Эта часть ещё не доведена до полного production workflow, но архитектурное направление уже зафиксировано.

