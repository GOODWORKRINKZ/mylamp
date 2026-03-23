# Frontend Dev Stub Backend Design

**Date:** 2026-03-23

## Goal

Добавить локальный запуск frontend через Vite dev server без железа, со стабовым backend и переключаемыми сценариями состояния лампы.

## Decision

Использовать Vite dev server с mock HTTP middleware, а не отдельный backend-процесс и не purely in-memory fetch monkeypatch.

## Why This Approach

- frontend продолжает использовать реальные `fetch("/api/...")` вызовы;
- dev-mode ближе к настоящему firmware HTTP API;
- сценарии можно переключать без поднятия отдельного Node-сервиса;
- позже можно смешивать mock и proxy на реальную лампу без переписывания UI-кода.

## Scope

Локальный dev backend должен отвечать на:

- `GET /api/status`
- `POST /api/live/validate`
- `POST /api/live/run`
- `GET /api/presets`
- `GET /api/presets/:id`
- `PUT /api/presets/:id`
- `DELETE /api/presets/:id`
- `POST /api/presets/:id/activate`
- `PUT /api/playlists/:id`
- `DELETE /api/playlists/:id`
- `POST /api/playlists/:id/start`
- `POST /api/playlists/stop`

Это покрывает текущий и ближайший frontend flow без полной эмуляции firmware.

## Scenario Model

В dev-mode вводятся переключаемые сценарии:

- `happy-path`
- `autoplay`
- `dsl-error`
- `offline-ish`
- `sensor-missing`

Сценарий выбирается по приоритету:

1. query param `?scenario=...`
2. `localStorage`
3. default `happy-path`

## Runtime Shape

Mock runtime хранится только в памяти Vite dev process и содержит:

- status snapshot
- preset store
- playlist store
- active preset id
- active playlist id
- autoplay flag
- diagnostics summary

Стор обновляется через mock API handlers, чтобы UI мог жить как будто работает с реальным устройством.

## Frontend UX

В shell появляется dev-only control panel:

- selector сценария
- краткое описание сценария
- кнопка reset mock state

Переключение сценария должно:

- записывать новое значение в `localStorage`
- вызывать обновление `/api/status`
- переинициализировать in-memory mock state

## Error Handling

- `dsl-error` сценарий возвращает structured diagnostics для `validate` и `run`
- `offline-ish` сценарий может возвращать degraded status без падения dev server
- неизвестный scenario fallback'ается в `happy-path`

## Verification

- `npm run build` должен оставаться зелёным
- `npm run dev` должен поднимать локальный frontend
- UI должен работать без ESP32
- scenario switch должен менять данные `/api/status` и ответов live/preset/playlist endpoints
