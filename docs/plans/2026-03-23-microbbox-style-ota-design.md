# Microbbox-Style OTA Design

**Date:** 2026-03-23

## Goal

Перенести в `mylamp` firmware OTA-механику по образцу `microbbox` с теми же базовыми принципами:

- GitHub Releases как источник обновлений;
- каналы `stable` и `dev`;
- версия сборки из Git и канала;
- release/dev CI workflows;
- архитектура, рассчитанная на несколько hardware targets, даже если сейчас активен только один.

## Scope

В этой фазе переносится именно firmware/update pipeline и связанный release process.

В scope входят:

- build identity для прошивки (`version`, `channel`, `board`, `hardware type`);
- OTA client в прошивке;
- поиск релизов в GitHub Releases;
- установка обновления по HTTPS OTA;
- firmware update API для встроенного web UI;
- GitHub Actions для CI, dev builds и release builds;
- схема веток и naming артефактов, совместимая с `microbbox`.

Вне scope:

- перенос любой не-OTA логики `microbbox`, не относящейся к обновлениям;
- автоматическая установка обновлений без действия пользователя;
- полный рефактор всей архитектуры `mylamp` под многопродуктовую платформу.

## Architecture

### Firmware identity

Прошивка должна публиковать и использовать четыре независимых атрибута:

- `version` — semver или dev build id;
- `channel` — `stable` или `dev`;
- `board` — техническое имя target/platform;
- `hardwareType` — стабильный идентификатор железки для подбора OTA assets.

Сейчас `hardwareType` будет один, например `c3-cylinder32x16`, но он должен стать равноправной частью build-time identity. Новые железки в будущем добавляются через новые PlatformIO env и новый `hardwareType`, без смены OTA-протокола.

### Runtime components

В `mylamp` добавляется отдельный слой update runtime:

- `FirmwareReleaseInfo` — описание доступного обновления;
- `FirmwareUpdateClient` — запрос GitHub API и поиск совместимого релиза;
- `FirmwareUpdater` — скачивание бинарника и запись через ESP OTA API;
- `FirmwareUpdateService` — orchestration layer для `check` и `install`;
- `UpdateStatusSnapshot` или расширение существующего status snapshot — текущее состояние update subsystem.

Текущий `BuildInfo` сохраняется как источник compile-time identity, но расширяется полями для OTA.

### Web/API integration

OTA не должен жить отдельно от текущего firmware status flow. Update state встраивается в web API и UI через новые маршруты и/или расширение существующего snapshot.

Минимальный API:

- `GET /api/update/status`
- `POST /api/update/check`
- `POST /api/update/install`

При необходимости прогресс установки может возвращаться либо из `GET /api/update/status`, либо через отдельный progress endpoint.

## Versioning And Channels

### Stable releases

Stable releases используют semver tag уровня продукта:

- `v0.1.0`
- `v0.1.1`
- `v0.2.0`

Для каждой железки в релиз загружается отдельный бинарник.

Шаблон имени:

`mylamp-{hardware}-v{version}-release.bin`

Пример:

`mylamp-c3-cylinder32x16-v0.1.0-release.bin`

### Dev builds

Dev builds совместимы по идее с `microbbox`:

- branch name;
- short commit hash;
- timestamp.

Шаблон имени:

`mylamp-{hardware}-dev-{build}.bin`

Пример:

`mylamp-c3-cylinder32x16-dev-develop-a1b2c3d-20260323-204500.bin`

### Checksums and metadata

Рядом с каждым `.bin` публикуется `.sha256`.

Checksum нужен сразу, даже если на первом шаге прошивка будет использовать его частично. Это позволит не менять release contract позже.

## GitHub Releases Contract

### Release source

Прошивка использует GitHub Releases напрямую как upstream update source.

Для `stable`:

- published releases с semver tags.

Для `dev`:

- prerelease/dev releases с dev build id.

### Asset selection

Устройство фильтрует релиз по каналу, затем выбирает asset строго по `hardwareType`.

Отсутствие asset для текущей железки — это ошибка совместимости релиза, а не отсутствие обновлений.

## Device Data Flow

### Check for updates

1. Устройство знает `version`, `channel`, `board`, `hardwareType`.
2. По ручному запросу или по расписанию делает запрос к GitHub Releases API.
3. Для `stable` ищет последний подходящий stable release.
4. Для `dev` ищет последний подходящий prerelease/dev release.
5. В релизе ищет asset по hardware-specific имени.
6. Если найден совместимый бинарник новее текущего, формирует `FirmwareReleaseInfo`.
7. Отдает в UI/API результат: обновление найдено, не найдено или произошла ошибка проверки.

### Install update

1. UI/API инициирует установку.
2. Прошивка валидирует выбранный release и asset.
3. Запускается HTTPS OTA download.
4. Бинарник пишется через штатный ESP OTA flow.
5. После успешной записи выполняется завершение OTA и reboot.
6. После перезапуска устройство отдает новую версию в status snapshot.

## Failure Handling

Обязательные свойства системы:

- отказ GitHub API не ломает основное приложение;
- отсутствие подходящего asset для текущей железки возвращается как отдельная ошибка;
- checksum mismatch прерывает установку до reboot;
- установка должна использовать стандартный безопасный OTA path ESP32;
- `dev` канал маркируется как нестабильный в UI;
- update install запускается только вручную пользователем.

Автоматический background install не входит в текущую фазу.

## GitHub Actions And Release Flow

### Workflow set

В репозиторий переносятся workflow того же класса, что и в `microbbox`:

- `ci.yml`
- `dev-build.yml`
- `release.yml`
- `branch-protection.yml`
- `create-support-branch.yml`
- `backport-hotfix.yml`
- `version-support-check.yml`

### CI

`ci.yml`:

- запускается на `push` и `pull_request`;
- собирает firmware и гоняет тесты;
- становится required check для основных веток.

### Dev build

`dev-build.yml`:

- запускается на `develop`, `feature/*` и вручную;
- собирает dev firmware для всех активных hardware targets;
- публикует artifacts;
- опционально создает draft/prerelease для OTA dev канала.

### Release

`release.yml`:

- запускается вручную для `major` / `minor` / `patch`;
- вычисляет следующую версию;
- собирает release assets для всех hardware targets;
- создает draft GitHub Release;
- загружает `.bin` и `.sha256`;
- публикует релиз только после успешной полной сборки.

## Branching Model

Веточная модель копируется по идее из `microbbox`:

- `main`
- `develop`
- `feature/*`
- `release/*`
- `support/*`
- `hotfix/*`

Даже при одном текущем target это полезно, потому что release/support lifecycle и OTA channels должны быть устойчивы к появлению новых железок.

## Hardware Scalability

Система проектируется как multi-hardware с одним текущим target.

Добавление новой железки в будущем должно требовать только:

- нового PlatformIO environment;
- нового `hardwareType`;
- добавления target в workflow matrix;
- публикации нового hardware-specific binary.

OTA client, release contract и naming rules при этом не меняются.

## Testing Strategy

Обязательные уровни тестирования:

- unit tests для парсинга release metadata и выбора asset по `hardwareType`;
- unit tests для version/build id generation logic;
- unit tests для update status serialization;
- build verification для dev/release targets;
- workflow verification на уровне generated assets и naming contract.

Интеграционные сетевые проверки допустимо ограничить моками/фикстурами JSON от GitHub API, чтобы не делать тесты хрупкими.

## Recommendation

Реализация должна идти в таком порядке:

1. build identity и version generation;
2. OTA domain model и parser release metadata;
3. firmware update service и web API;
4. UI integration;
5. GitHub Actions dev/release workflows;
6. branch/support automation;
7. end-to-end verification на реальном устройстве.