# Release Contract

Этот документ фиксирует, какие OTA-артефакты публикуются, как они именуются и какой workflow считается валидным для `dev` и `stable` каналов.

## Каналы

Проект использует два OTA-канала:

- `dev` — prerelease-сборки для тестирования новых изменений;
- `stable` — релизные сборки по git tag.

## Что публикуется

Для каждого OTA-кандидата публикуются два файла:

1. бинарник прошивки `.bin`;
2. checksum-файл `.sha256`.

Других обязательных release-артефактов сейчас нет.

## Hardware Type

Текущий release contract описан для одного типа устройства:

- `c3-cylinder32x16`

Именно этот идентификатор используется в именах release-артефактов.

## Stable Release Flow

Stable-релиз собирается workflow [release.yml](../.github/workflows/release.yml).

Триггер:

- push git tag формата `v*.*.*`

Собираемое окружение:

- `esp32-c3-supermini-release`

Имя бинарника:

- `mylamp-c3-cylinder32x16-<tag>-release.bin`

Имя checksum:

- `mylamp-c3-cylinder32x16-<tag>-release.bin.sha256`

Пример:

- `mylamp-c3-cylinder32x16-v0.1.0-release.bin`
- `mylamp-c3-cylinder32x16-v0.1.0-release.bin.sha256`

Публикация:

- GitHub Release создаётся или обновляется автоматически;
- release notes генерируются GitHub Actions.

## Dev Release Flow

Dev-сборки публикуются workflow [dev-build.yml](../.github/workflows/dev-build.yml).

Триггеры:

- push в `develop`
- push в `feature/**`
- ручной `workflow_dispatch`

Собираемое окружение:

- `esp32-c3-supermini-dev`

Формат версии dev-сборки:

- `<branch>-<short_sha>-<utc_timestamp>`

Имя бинарника:

- `mylamp-c3-cylinder32x16-dev-<build_version>.bin`

Имя checksum:

- `mylamp-c3-cylinder32x16-dev-<build_version>.bin.sha256`

Публикация:

- как workflow artifact;
- как GitHub prerelease с тегом `dev-<build_version>`.

## Что считается валидным OTA-кандидатом

Валидный кандидат обновления должен удовлетворять всем условиям:

1. Есть `.bin` файл для нужного hardware type.
2. Есть matching `.sha256` файл.
3. Имя артефакта соответствует каналу и ожидаемому шаблону.
4. Артефакт собран GitHub Actions из текущего репозитория, а не вручную вне CI.

Если хотя бы одно условие не выполнено, такой артефакт не должен считаться production-grade OTA-кандидатом.

## Локальная проверка перед релизом

Минимальная проверка перед публикацией:

1. `npm ci` в `frontend/`
2. `~/.platformio/penv/bin/platformio test -e native-test`
3. `~/.platformio/penv/bin/platformio run -e esp32-c3-supermini-dev`
4. `~/.platformio/penv/bin/platformio run -e esp32-c3-supermini-release`

Если менялись embedded assets, сборка firmware сама подтянет frontend build через pre-build шаг.

## Выпуск stable-релиза

Порядок действий:

1. Убедиться, что `main` в нужном состоянии.
2. Прогнать локальные проверки.
3. Создать тег вида `v0.1.0`.
4. Запушить тег.
5. Проверить, что workflow `Release Build` опубликовал `.bin` и `.sha256`.

## Выпуск dev-сборки

Порядок действий:

1. Запушить ветку `develop` или `feature/*`.
2. Дождаться workflow `Dev Build`.
3. Проверить workflow artifact и prerelease с dev-тегом.

## Связь с OTA UI

Frontend OTA UI исходит из следующего контракта:

- `check` сообщает, есть ли более новая версия на выбранном канале;
- `install` ставит найденный релиз;
- после `install` устройство может быть временно недоступно из-за reboot;
- checksum mismatch, network failure и release fetch errors считаются штатными диагностируемыми ошибками.

Если naming release-артефактов или стратегия публикации изменится, этот файл и OTA backend нужно обновлять вместе.